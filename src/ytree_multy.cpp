//
// Created by trast on 28/12/2022.
//
#include <yocto/yocto_cli.h>
#include <yocto/yocto_gui.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_trace.h>

#include <iostream>

#include "treeToolSet.cpp"
#include "yocto/yocto_modelio.h"

using namespace yocto;
using namespace std::string_literals;

// main function
void run(const vector<string>& args) {
  // parameters
  auto scenename   = "scene.json"s;
  auto outname     = "point_image.png"s;
  auto paramsname  = ""s;
  auto interactive = false;
  auto edit        = false;
  auto camname     = ""s;
  bool addsky      = false;
  auto envname     = ""s;
  auto savebatch   = false;
  auto dumpname    = ""s;
  //custom
  auto rnd_input = false;
  auto num_attrPoint = ""s;
  auto attr_range = ""s;
  auto kill_range = ""s;
  auto treeType = ""s;

  //
  auto params      = trace_params{};
  // parse command line
  auto cli = make_cli("ytree", "render a procedural tree with raytracing");
  add_option(cli, "scene", scenename, "scene filename");
  add_option(cli, "output", outname, "output filename");
  add_option(cli, "params", paramsname, "params filename");
  add_option(cli, "interactive", interactive, "run interactively");
  add_option(cli, "camera", camname, "camera name");
  add_option(cli, "addsky", addsky, "add sky");
  add_option(cli, "envname", envname, "add environment");
  add_option(cli, "savebatch", savebatch, "save batch");
  add_option(cli, "resolution", params.resolution, "image resolution");
  add_option(
      cli, "sampler", params.sampler, "sampler type", trace_sampler_labels);
  add_option(cli, "falsecolor", params.falsecolor, "false color type",
      trace_falsecolor_labels);
  add_option(cli, "samples", params.samples, "number of samples");
  add_option(cli, "bounces", params.bounces, "number of bounces");
  add_option(cli, "batch", params.batch, "sample batch");
  add_option(cli, "clamp", params.clamp, "clamp params");
  add_option(cli, "nocaustics", params.nocaustics, "disable caustics");
  add_option(cli, "envhidden", params.envhidden, "hide environment");
  add_option(cli, "tentfilter", params.tentfilter, "filter image");
  add_option(cli, "noparallel", params.noparallel, "disable threading");
  add_option(cli, "dumpparams", dumpname, "dump params filename");
  add_option(cli, "edit", edit, "edit interactively");
  // custom
  add_option(cli, "random", rnd_input, "set true to generate a random seed for attraction points");
  add_option(cli, "numattr", num_attrPoint, "define the number of attraction point to generate");
  add_option(cli, "tree", treeType, "define type of tree");
  add_option(cli, "attr_range", attr_range, "define an attraction range for a branch");
  add_option(cli, "kill_range", kill_range, "define killing range for attraction points");
  //
  parse_cli(cli, args);
  // load config
  if (!paramsname.empty()) {
    update_trace_params(paramsname, params);
    print_info("loading params {}", paramsname);
  }

  // dump config
  if (!dumpname.empty()) {
    save_trace_params(dumpname, params);
    print_info("saving params {}", dumpname);
  }

  // start rendering
  print_info("rendering {}", scenename);
  auto timer = simple_timer{};

  // scene loading
  timer      = simple_timer{};
  auto scene = load_scene(scenename);  // carico la shape
  print_info("load scene: {}", elapsed_formatted(timer));

  //////////////////////////////////////////////////////////////////////////////
  //  UTILS
  //  To print any object from vectors
  //  int i = 0;
  //  while (i < scene.instance_names.size()) {
  //    print_info("instaces_name: {}", scene.instance_names[i]);
  //    i++;
  //  }
  //////////////////////////////////////////////////////////////////////////////
  /* individua la mash del punto e salvala in una var
   * individua le coordinate relative a tale punto e salvale in una var
   * per i che va da 1 a 10
   *    incrementa il valore di z delle coordinate della mash
   *    crea una nuova instanza del modello usato come punto
   *    inserisci le  coordinate nella relativa struttura dati
   *
   *    frame3f frame
   *    int     shape
   *    int     material
   *
   */
  /*
   * Procedura di crezione istanze:
   *  1) Caricare modelli e texture nel .json e individuare la posizione al interno dell'array;
   *  2) Creare una instanza standard di instance_data{} con un fram3f di default e i corretti indici di modelli e texture;
   *  3) Itera sull'array dei vettori
   *        Copia la instanza standard;
   *        Modifica l'instanza standard con il vettore posizione corretto (scene.frame.o);
   *        Inserisci la nuova istanza nella scena;
   */
  // INSTANCES
  attractionPoints crown;
  crown.radiusInfluence = stof(attr_range);
  crown.killDistance = stof(kill_range);
  vector<Branch> branchesArray; // a collection of all branches in the tree
  vector<Branch> extremities;   // a collection of branches that have no childrens
  // GENERATE THE CROWN OF ATTRACTION POINT
  // seeding
  auto seedrnd = randomSeed(rnd_input, 0, 100000);
  print_info("uniform seeding: {}, no random seed -> 58380", seedrnd);
  // genertion
  crown.attractionPointsArray = populateSphere(stoi(num_attrPoint), seedrnd);
  // sort attraction points vertically
  quicksort_attrPoint3f(crown.attractionPointsArray, 0, crown.attractionPointsArray.size());
  // l'attraction Point piu' basso.
  auto minVec3f = crown.attractionPointsArray[0];
  print_info("minYvec3f= {}, {}, {}", minVec3f.coords.x, minVec3f.coords.y, minVec3f.coords.z);
  // modeling for attrPoints
  float ModelScale = 0.4;
  auto floorPos = scene.instances[0].frame.o;
  print_info("floorPos= {},{},{}", floorPos.x, floorPos.y, floorPos.z);
  auto  attractionPointInstance = instance_data{frame3f{{ModelScale,0,0},
                                                              {0,ModelScale,0},
                                                              {0,0,ModelScale},
                                                              {0,0,0}},
                                                        1, 1};

  // TREE TRUNK
  // branch render instance
  auto branchInstanceData = instance_data{frame3f{{ModelScale,0,0},
                                                  {0,ModelScale,0},
                                                  {0,0,ModelScale},
                                                  floorPos}, 2, 2};
  // trunk instance and growth direction
  vec3f trunkGrowthDir = {0, 0.5, 0};
  auto trunkBranch = Branch{floorPos,
      floorPos += trunkGrowthDir,
      trunkGrowthDir *= 0.03,
      0.03,
      NULL,
      vector<Branch>(),
      vector<attrPoint3f>(),
      vector<int>(),
      0.1,
      100,
      true};

  auto growingTrunk = insertChildBranch(trunkBranch, trunkGrowthDir);
//  print_info("runningBranch: ({}, {}, {})", runningBranch._end.x, runningBranch._end.y, runningBranch._end.z);
  branchesArray.push_back(trunkBranch);
  branchesArray.push_back(growingTrunk);
  while (checkHeight(growingTrunk, minVec3f.coords)){
    // data
    growingTrunk = insertChildBranch(trunkBranch, vec3f{0, 1, 0});
    trunkBranch = growingTrunk;
    branchesArray.push_back(growingTrunk);
    // models
    auto b_instance = branchInstanceData;
    b_instance.frame.o = trunkBranch._start;
    scene.instances.push_back(b_instance);
  }

  while (crown.attractionPointsArray.size() > 0){
    for (Branch currentBranch : branchesArray){
      vec3f dir;
      auto exception_caught = false;
      print_info("currentBranch= ({}, {}, {})", currentBranch._start.x, currentBranch._start.y, currentBranch._start.z);
      try{
        findInfluenceSet(currentBranch, crown);
      } catch (const NoInfluencePointsInRange& err){
        cout << err.message << endl;
        dir = rndDirection(currentBranch, seedrnd);
        exception_caught = true;
      }
      if (exception_caught) {
        dir = computeDirection(currentBranch, seedrnd);
      }
      auto nextBranch = insertChildBranch(currentBranch, dir);
      print_info("nextBranch= ({}, {}, {})", nextBranch._start.x, nextBranch._start.y, nextBranch._start.z);
      deleteAttractionPoints(currentBranch, crown);
      branchesArray.push_back(nextBranch);
      // Models
      auto b_instance    = branchInstanceData;
      b_instance.frame.o = currentBranch._start;
      scene.instances.push_back(b_instance);
    }
  }
//  // MODELS
//  auto b_instance    = branchInstanceData;
//  b_instance.frame.o = growingBranch._start;
//  scene.instances.push_back(b_instance);
//  //
  // MODELS
  for (auto ap : crown.attractionPointsArray){
    auto aP_instance = attractionPointInstance;
    aP_instance.frame.o = ap.coords;
    scene.instances.push_back(aP_instance);
  }

  //////////////////////////////////////////////////////////////////////////////

  // build bvh
  timer    = simple_timer{};
  auto bvh = make_trace_bvh(scene, params);
  print_info("build bvh: {}", elapsed_formatted(timer));

  auto lights = make_trace_lights(scene, params);

  // fix renderer type if no lights
  if (lights.lights.empty() && is_sampler_lit(params)) {
    print_info("no lights presents, image will be black");
    params.sampler = trace_sampler_type::eyelight;
  }

  // add sky
  if (addsky) add_sky(scene);

  // add environment
  if (!envname.empty()) {
    add_environment(scene, envname);
  }

  // camera
  params.camera = find_camera(scene, camname);

  // tesselation
  if (!scene.subdivs.empty()) {
    tesselate_subdivs(scene);
  }

  // execs
  auto state = make_trace_state(scene, params);

  // init renderer
  if (!interactive) {
    // render
    timer = simple_timer{};
    for (auto sample : range(0, params.samples, params.batch)) {
      auto sample_timer = simple_timer{};
      trace_samples(state, scene, bvh, lights, params);
      print_info("render sample {}/{}: {}", state.samples, params.samples,
          elapsed_formatted(sample_timer));
      if (savebatch && state.samples % params.batch == 0) {
        auto image     = get_image(state);
        auto batchname = replace_extension(outname,
            "-" + std::to_string(state.samples) + path_extension(outname));
        save_image(batchname, image);
      }
    }
    print_info("render image: {}", elapsed_formatted(timer));

    // save image
    timer      = simple_timer{};
    auto image = get_image(state);
    save_image(outname, image);
    print_info("save image: {}", elapsed_formatted(timer));
  } else {
#ifdef YOCTO_OPENGL
    // rendering context
    auto context = make_trace_context(params);

    // init image
    auto image = make_image(state.width, state.height, true);

    // opengl image
    auto glimage     = glimage_state{};
    auto glparams    = glimage_params{};
    glparams.tonemap = true;

    // camera names
    auto camera_names = scene.camera_names;
    if (camera_names.empty()) {
      for (auto idx : range(scene.cameras.size())) {
        camera_names.push_back("camera" + std::to_string(idx + 1));
      }
    }

    // start rendering batch
    auto render_next = [&]() {
      trace_cancel(context);
      trace_start(context, state, scene, bvh, lights, params);
    };

    // restart renderer
    auto render_restart = [&]() {
      // make sure we can start
      trace_cancel(context);
      state = make_trace_state(scene, params);
      if (image.width != state.width || image.height != state.height)
        image = make_image(state.width, state.height, true);

      // render preview
      trace_preview(image, context, state, scene, bvh, lights, params);

      // update image
      set_image(glimage, image);

      // start
      trace_start(context, state, scene, bvh, lights, params);
    };

    // render cancel
    auto render_cancel = [&]() { trace_cancel(context); };

    // render update
    auto render_update = [&]() {
      if (context.done) {
        get_image(image, state);
        set_image(glimage, image);
        trace_start(context, state, scene, bvh, lights, params);
      }
    };

    // prepare selection
    auto selection = scene_selection{};

    // callbacks
    auto callbacks = gui_callbacks{};
    callbacks.init = [&](const gui_input& input) {
      init_image(glimage);
      render_restart();
    };
    callbacks.clear = [&](const gui_input& input) { clear_image(glimage); };
    callbacks.draw  = [&](const gui_input& input) {
      render_update();
      update_image_params(input, image, glparams);
      draw_image(glimage, glparams);
    };
    callbacks.widgets = [&](const gui_input& input) {
      auto tparams = params;
      if (draw_trace_widgets(input, state.samples, tparams, camera_names)) {
        render_cancel();
        params = tparams;
        render_restart();
      }
      draw_tonemap_widgets(input, glparams.exposure, glparams.filmic);
      draw_image_widgets(input, image, glparams);
      if (edit) {
        if (draw_scene_widgets(scene, selection, render_cancel)) {
          render_restart();
        }
      }
    };
    callbacks.uiupdate = [&](const gui_input& input) {
      auto camera = scene.cameras[params.camera];
      if (uiupdate_camera_params(input, camera)) {
        render_cancel();
        scene.cameras[params.camera] = camera;
        render_restart();
      }
    };

    // run ui
    show_gui_window({1280 + 320, 720}, "ytrace - " + scenename, callbacks);
    // done
    render_cancel();
#else
    throw io_error{"Interactive requires OpenGL"};
#endif
  }
}

// Run
int main(int argc, const char* argv[]) {
  try {
    run({argv, argv + argc});
    return 0;
  } catch (const std::exception& error) {
    print_error(error.what());
    return 1;
  }
}
