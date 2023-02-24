#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "yocto/yocto_math.h"
#include "yocto/yocto_shape.h"
#include "yocto/yocto_trace.h"

using namespace std;
using std::string;
using std::vector;
namespace yocto {

// STRUCTS


typedef struct attrPoint3f {
  vec3f coords;
  int ID;
}attrPoint3f;


typedef struct attractionPoints {
  // Conteiner per la corona di punti di attrazione
  vector<attrPoint3f> attractionPointsArray;  // vettore di attraction points in uno spazio
  string treeCrownShape;  // directoy del modello blender per la forma della chioma
  float  radiusInfluence;  // raggio di influenza di ogni punto di attrazione
  float  killDistance;     // raggio di eliminazione dei punti di influenza
  shape_data* shapeIndex; // Indice riferito al json della scena che indica la posizione del modello.
} attractionPoints;


typedef struct Branch {
  // singolo nodo dell'albero
  vec3f     _start;        // coordinate spaziali del nodo
  vec3f     _end;
  vec3f     _direction;
  float     _length;
  Branch*   father_ptr{};  // puntatore al branch padre
  vector<Branch> children;
  vector<attrPoint3f> influencePoints;
  vector<int>         influenceIDs;
  int   depth; //numero di discendenti
  int   maxBranches; // definisce il numero massimo di figli generabili
  int   minBranches; // definisce il numero minimo di figli da generare
  bool  fertile;      // definisce se il nodo è fertile o meno
  float trunkDiameter; // Diametro del tronco sul punto.
  bool  leaf;
} Branch;

// ERRORS

class NoFertileBranch : public runtime_error{
 public:
  NoFertileBranch(const string& arg) : runtime_error(arg){}
};

class NoInfluencePointsInRange : public runtime_error{
 public:
  NoInfluencePointsInRange(const string& arg): runtime_error(arg) {}
};

class TreeNotDefined : public runtime_error{
 public:
  TreeNotDefined(const string& arg) : runtime_error(arg) {}
};

// UTIL FUNCTIONS
int randomSeed(bool random, int interval_start, int interval_end) {
    if (!random) {
        return 58380;
    } else {
        random_device generator;  // non deterministic, if deterministic use
        mt19937 rdm(generator());
        uniform_int_distribution<int> intDistribution(interval_start, interval_end);
        return intDistribution(rdm);
    }
}


vector<attrPoint3f> populateSphere(int num_points, int seed) {
  auto rng = make_rng(seed); // seed the generator
  vector<attrPoint3f> rdmPoints_into_sphere;
  int ID = 0;
  for (auto i : range(num_points)){
        rdmPoints_into_sphere.push_back(attrPoint3f{sample_sphere(rand2f(rng)), ID});
        ID++;
  }
  return rdmPoints_into_sphere;
}



float scaleTrunkDiameter(float previousDiameter, string treeName){
  /*  Scala secondo una percentuale fissa il diametro di ogni treeNode
   *  Imposta un rateo per tipologia di albero
   */
  if (treeName == "default") {
        float scale = 0.01;
        if (previousDiameter > 0){
          float newDiameter;
          newDiameter = previousDiameter-(previousDiameter*scale);
          return newDiameter;
        }
  }

  if (treeName == "tree2") {
        float value = 0.5;
        if (previousDiameter > 0){
         float newDiameter;
         newDiameter = previousDiameter-value; //diametro scalato di una quantita fissa
         return newDiameter;
        }
  }

  return -1; // treetype not defined lancia un errore
}


float scaleLength(float previousLength, string treeName){
  if (treeName == "default") {
        float scale = 0.01;
        if (previousLength > 0){
         return previousLength - (previousLength * scale);
        }
  }

  if (treeName == "tree2") {
        float value = 0.5;
        if (previousLength > 0){
         return previousLength - value;
        }
  }
  throw TreeNotDefined("Tree was not defined");
}


vec3f populateMash(); //TODO: SAMPLE MASH

//vector<vec3f> populateCube_exclusion(float diagonal, vec3f center, int num_points) {
//  random_device generator;  // non deterministic, if deterministic use
//                            // marsenne-twisted-generator
//  uniform_real_distribution<float> uniformRealDistribution(1.0, 500.0);  // maybe add a max value as radius
//  vector<vec3f> rdmPoints_into_cube;
//  int           i = 0;
//
//  int seed = randomSeed(true, 1, 500);
//  // CHIAMO FUNZIONE PROFESSORE PER GENERARE IL CUBO
//  //  Make random points in a cube. Returns points, pos, norm, texcoord, radius.
//  shape_data cubeShape = make_random_points(65536, {1, 1, 1}, 1, 0.001f,
//      seed);  // cambiare tramite funzione in true e false
//
//  /*while (i < num_points) {
//    float x     = uniformRealDistribution(generator);
//    float y     = uniformRealDistribution(generator);
//    float z     = uniformRealDistribution(generator);
//    vec3f point = {x, y, z};
//    float delta = distance(point, center);
//
//
//    alfa=cubeside/2; //DEFINIRE CUBESIDE NELLA STRUCT DEL CUBO
//    //            float delta = pow((x - center.x), 2) + pow((y - center.y), 2)
//    //            + pow((z - center.z),2 );
//    if (x<(center.x+alfa)&&x>(center.x-alfa)) { //controllo se il punto è
//    all'interno del cubo if(x<(center.y+alfa)&&x>(center.y-alfa)) { //controllo
//    su ogni lato per cooridnate if(x<(center.z+alfa)&&x>(center.z-alfa)) {
//          rdmPoints_into_cube.push_back(point);
//          i++;
//          }
//        }
//      } //parentesi if*/
//  return
//}


// SPATIAL COLONIZATION FUNCTIONS
// Simple checks

bool checkHeight(Branch current, vec3f minVec) { return current._end.y < minVec.y; }


bool isFertile(Branch& current){
  if (current.depth > 0 &&
      current.children.size() < current.maxBranches &&
      !current.influencePoints.empty()){
        return true;
  }
  return false;
}
bool checkGlobalFertility(vector<Branch>& branchesArray){
  for (auto& b : branchesArray){
    if (b.fertile){
      return true;
    }
  }
  return false;
}

// Structs modifications

void findInfluenceSet(Branch& current_branch, attractionPoints& treeCrown) {
  double radiusInfluence = treeCrown.radiusInfluence;
  for (auto ap :treeCrown.attractionPointsArray) {
//    auto d = sqrt(sqr((ap.x - current_branch._end.x)) + sqr((ap.x - current_branch._end.x)) + sqr((ap.x - current_branch._end.x)));
    double d = distance(current_branch._end, ap.coords);
    if (d <= radiusInfluence) {
       current_branch.influencePoints.push_back(ap);
       current_branch.influenceIDs.push_back(ap.ID);
    }
  }
  if (current_branch.influencePoints.empty()){
    throw NoInfluencePointsInRange("No attraction points in range"s);
  }
}


vec3f computeDirection(Branch& fatherBranch, int seed){
  rng_state rng = make_rng(seed);
  vec3f newDir = {0, 0, 0};
  vec3f num;
  float denom;
  for (auto& ip : fatherBranch.influencePoints) {
    num = ip.coords -= fatherBranch._end;
    denom  = sqrt(dot(num, num));
    newDir = newDir += vec3f{num.x / denom, num.y / denom, num.z / denom};
  }
  newDir = newDir /= fatherBranch.influencePoints.size();
  auto newDir_norm = sqrt(dot(newDir, newDir));
  return (newDir /= newDir_norm) += rand3f(rng);
}


vec3f rndDirection(Branch& fatherBranch, int seed){
  rng_state rng = make_rng(seed);
  return fatherBranch._direction += rand3f(rng);
}


Branch growChildBranch(Branch& fatherBranch, vec3f direction){
    Branch newBranch;
    newBranch._start      = fatherBranch._end;
    newBranch._direction  = direction *= fatherBranch._length;
    newBranch._end = newBranch._direction += newBranch._start;
    newBranch._length     = fatherBranch._length;
    newBranch.father_ptr  = &fatherBranch;
    newBranch.maxBranches = fatherBranch.maxBranches;
    newBranch.minBranches = fatherBranch.minBranches;
    newBranch.depth = fatherBranch.depth - 1;
    fatherBranch.children.push_back(newBranch);
    return newBranch;
}

Branch growChildTrunk(Branch& fatherBranch, vec3f direction){
    Branch newBranch;
    newBranch._start      = fatherBranch._end;
    newBranch._direction  = direction *= fatherBranch._length;
    newBranch._end = newBranch._direction += newBranch._start;
    newBranch._length     = fatherBranch._length;
    newBranch.father_ptr  = &fatherBranch;
    newBranch.maxBranches = fatherBranch.maxBranches;
    newBranch.minBranches = fatherBranch.minBranches;
    newBranch.depth = 0;
    fatherBranch.children.push_back(newBranch);
    return newBranch;
}

void clearInfluenceSet(Branch& branch){
    branch.influencePoints.clear();
    branch.influenceIDs.clear();
}


void deleteAttractionPoints(Branch& current, attractionPoints& treeCrown){
    auto killDistance = treeCrown.killDistance;
    for (int i : range(current.influencePoints.size())){
      double d = distance(current._end, current.influencePoints[i].coords);
      if (d <= killDistance){
        auto ID = current.influenceIDs[i];
        for (int ap : range(treeCrown.attractionPointsArray.size())){
          if (ID == treeCrown.attractionPointsArray[ap].ID){
            treeCrown.attractionPointsArray.erase(treeCrown.attractionPointsArray.begin() + ap);
          }
        }
      }
    }
}

int defertilize(Branch* current ){
    current -> fertile = false;
    if(!current -> fertile){
      return 1;
    }
    return 1 + defertilize(current -> father_ptr);
}

// Sorting algorithm
// Quicksort code from:
// https://www.geeksforgeeks.org/cpp-program-for-quicksort/ adapted to sort attrPoint3f
template <typename T>
void swap(vector<T>& array, int index1, int index2) {
  auto a        = array[index1];
  array[index1] = array[index2];
  array[index2] = a;
}


auto partition_attrPoint3f(vector<attrPoint3f>& arrVec3f, int start, int end) {
  float pivot = arrVec3f[start].coords.y;
  int   count = 0;
  for (int i = start + 1; i <= end; i++) {
        if (arrVec3f[i].coords.y <= pivot) count++;
  }
  // Giving pivot element its correct position
  int pivotIndex = start + count;
  swap(arrVec3f, pivotIndex, start);
  // Sorting left and right parts of the pivot element
  int i = start, j = end;
  while (i < pivotIndex && j > pivotIndex) {
        while (arrVec3f[i].coords.y <= pivot) { i++;}
        while (arrVec3f[j].coords.y > pivot) { j--; }

        if (i < pivotIndex && j > pivotIndex) {
            swap(arrVec3f, i++, j--);
        }
  }
  return pivotIndex;
}


auto quicksort_attrPoint3f(vector<attrPoint3f>& vec, int start, int end) {
  if (start >= end) return;
  // partitioning the array
  auto p = partition_attrPoint3f(vec, start, end);
  quicksort_attrPoint3f(vec, start, p - 1);  // Sorting the left part
  quicksort_attrPoint3f(vec, p + 1, end);    // Sorting the right part
}
//

}  // namespace yocto

