//
// Created by tommasomarialopedote on 10/03/23.
//

#include <yocto/yocto_cli.h>
#include <yocto/yocto_gui.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>
#include <yocto/yocto_trace.h>
#include <yocto/yocto_modelio.h>

#include <iostream>

#include "treeToolSet.cpp"

using namespace yocto;
using namespace std::string_literals;

// main function
void run(const vector<string>& args) {
  // parameters
//  auto scenename = "/home/tommasomarialopedote/Computer-graphics-project/yocto-AlberoProcedurale/tests/tests_assets/cylinder_test/cylinder_test.json"s;
  auto scenename = R"(C:\yocto-AlberoProcedurale\tests\tests_assets\cylinder_test\cylinder_test.json)"s;
  auto outname     = "point_image.png"s;
  auto paramsname  = ""s;
  auto interactive = true;
  auto edit        = false;
  auto camname     = ""s;
  bool addsky      = false;
  auto envname     = ""s;
  auto savebatch   = false;
  auto dumpname    = ""s;

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
  // random generator
  random_device rdmGenerator;
  mt19937 rdm(rdmGenerator());
  auto seedrnd = randomSeed(true, 0, 100000, rdm);
  rng_state rng = make_rng(seedrnd);
  //
  vec3f bs1 = {0,-1.2,0};
  vec3f be1 = {0,-1 ,0};
  vec3f v1 = (be1 - bs1) / yocto::sqrt(dot((be1 - bs1),(be1 - bs1)));
  float d1 = distance(bs1, be1) * 1.8;

  vec3f bs2 = be1;
  vec3f be2 = {0.2, -0.9, 0.1};
  vec3f v2 = (be2 - bs2) / yocto::sqrt(dot((be2 - bs2), (be2 - bs2)));
  float d2 = distance(bs2, be2) * 1.8;
  vector<float> distances = {d1, d2};

  auto  attrInst1 = instance_data{frame3f{{0.25,0,0},
                                                   {0,0.25,0},
                                                   {0,0,0.25},
                                                   bs1},1, 2};
  auto  attrInst2 = instance_data{frame3f{{0.25,0,0},
                                      {0,0.25,0},
                                      {0,0,0.25},
                                     be1},1, 2};
  auto  attrInst3 = instance_data{frame3f{{0.25,0,0},
                                      {0,0.25,0},
                                      {0,0,0.25},
                                     bs2},1, 2};
  auto  attrInst4 = instance_data{frame3f{{0.25,0,0},
                                      {0,0.25,0},
                                      {0,0,0.25},
                                     be2},1, 2};
  scene.instances.push_back(attrInst1);
  scene.instances.push_back(attrInst2);
  scene.instances.push_back(attrInst3);
  scene.instances.push_back(attrInst4);

  auto angle1 = computeAngles({-1,0,-1}, v1);
  auto angle2 = computeAngles({-1,0,-1}, v2);

//  auto maxLength = maxInVector(distances);

  vec2f cy_scale1 = {0.1, d1};
  vec2f cy_scale2 = {0.1, d2};
  shape_data cy1 = make_uvcylinder({32,32,32}, cy_scale1);
  shape_data cy2 = make_uvcylinder({32,32,32}, cy_scale2);
  scene.shapes.push_back(cy1);
  scene.shapes.push_back(cy2);


  frame3f modForm1;
  vec3f middlePoint1 = {(bs1.x+be1.x)/2, (bs1.y+be1.y)/2, (bs1.z+be1.z)/2};
  auto translation1 = translation_frame(middlePoint1);
  auto scaling1 = scaling_frame({0.25, 0.25, 0.25});
  auto rotation1 = rotation_frame({1,0,0}, angle1.x) *
                   rotation_frame({0,1,0}, angle1.y) *
                   rotation_frame({0,0,1}, angle1.z);
  modForm1 = translation1 * rotation1 * scaling1;

  frame3f modForm2;
  vec3f middlePoint2 = {(bs2.x+be2.x)/2, (bs2.y+be2.y)/2, (bs2.z+be2.z)/2};
  auto translation2 = translation_frame(middlePoint2);
  auto scaling2 = scaling_frame({0.25, 0.25, 0.25});
  auto rotation2 = rotation_frame({1,0,0}, angle2.x) *
                   rotation_frame({0,1,0}, angle2.y) *
                   rotation_frame({0,0,1}, angle2.z);
  modForm2 = translation2 * rotation2 * scaling2;

  cout << "v1= " << v1.x << ", " << v1.y << ", " << v1.z << endl;
  cout << degrees(angle1.x) << ", " << degrees(angle1.y) << ", " << degrees(angle1.z) << endl;
  cout << "v2= "<< v2.x << ", " << v2.y << ", " << v2.z << endl;
  cout << degrees(angle2.x) << ", " << degrees(angle2.y) << ", " << degrees(angle2.z) << endl;


  instance_data cy_inst1 = {modForm1, 2,1};
  instance_data cy_inst2 = {modForm2, 3, 1};

  scene.instances.push_back(cy_inst1);
  scene.instances.push_back(cy_inst2);


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
