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

typedef struct attractionPoints {
  // Conteiner per la corona di punti di attrazione
  vector<vec3f> attractionPointsArray;  // vettore di attraction points in uno spazio
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
  float     _lenght;
  Branch*   father_ptr{};  // puntatore al branch padre
  vector<Branch> children;
  vector<vec3f> influencePoints;
  vector<int> indexesInfluence;
  float trunkDiameter{}; // Diametro del tronco sul punto.
  int   maxBranches{};  // definisce il numero massimo di figli generabili
  bool  fertile{};      // definisce se il nodo è fertile o meno
} Branch;


// UTIL FUNCTIONS
vec3f normalizeTree(vec3f v, float l) {
  if (v.x == 0 && v.y == 0 && v.z == 0){ return v; // NO! bad vector!
  } else {
    v.x *= l;
    v.y *= l;
    v.z *= l;
    return v;
  }
}


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


vector<vec3f> populateSphere(int num_points, int seed) {
  auto rng = make_rng(seed); // seed the generator
  vector<vec3f> rdmPoints_into_sphere;
  for (auto i : range(num_points)){
        rdmPoints_into_sphere.push_back(sample_sphere(rand2f(rng)));
  }
  return rdmPoints_into_sphere;
}



float scaleTrunkDiameter(float previousDiameter, string treeName){
  /*  Scala secondo ona percentuale fissa il diametro di ogni treeNode
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
bool checkHeight(Branch current, vec3f minVec) { return current._end.y < minVec.y; }

void findInfluenceSet(Branch& current_branch, attractionPoints& treeCrown) {
  auto attractionPointsArray = treeCrown.attractionPointsArray;
  double radiusInfluence = treeCrown.radiusInfluence;
  for (auto ap : range(attractionPointsArray.size())) {
//    auto d = sqrt(sqr((ap.x - current_branch._end.x)) + sqr((ap.x - current_branch._end.x)) + sqr((ap.x - current_branch._end.x)));
    double d = distance(current_branch._end, attractionPointsArray[ap]);
    if (d <= radiusInfluence) {
      current_branch.influencePoints.push_back(attractionPointsArray[ap]);
      current_branch.indexesInfluence.push_back(ap);
    }
  }
}

vec3f computeDirection(Branch& fatherBranch){
  if (fatherBranch.influencePoints.size() > 0) {
    vec3f newDir = {0, 0, 0};
    vec3f num;
    float denom;
    for (auto ip : fatherBranch.influencePoints) {
      num = ip - fatherBranch._end;
      denom  = sqrt(dot(num, num));
      newDir = newDir += vec3f{num.x / denom, num.y / denom, num.z / denom};
    }
    newDir = newDir /= fatherBranch.influencePoints.size();
    auto newDir_norm = sqrt(dot(newDir, newDir));
    return newDir /= newDir_norm;
  }
  return {0,0,0};
}


vec3f determined_direction(Branch& fatherBranch){
  auto fatherEnd = fatherBranch._end;
  return fatherEnd += vec3f{0, 0.15, 0.15};
}


bool checkFatherFertility(Branch& fBranch){
    if (fBranch.maxBranches > fBranch.children.size()){
        fBranch.fertile = true;
        return true;
    } else {
        fBranch.fertile = false;
        return false;
    }
}

Branch insertChildBranch(Branch& fatherBranch, vec3f direction){
    Branch newBranch;
    newBranch._start      = fatherBranch._end;
    newBranch._direction  = direction *= fatherBranch._lenght;
    newBranch._end = newBranch._direction  += newBranch._start;
    newBranch._lenght     = fatherBranch._lenght;
    newBranch.father_ptr  = &fatherBranch;
    newBranch.maxBranches = fatherBranch.maxBranches - 1;
    fatherBranch.children.push_back(newBranch);
    if (checkFatherFertility(fatherBranch)) {
        newBranch.fertile = true;
    } else {
        newBranch.fertile = false;
    }
    return newBranch;
}



void deleteAttractionPoints(Branch& current, attractionPoints& treeCrown){
    auto attractionArray = treeCrown.attractionPointsArray;
    auto killDistance = treeCrown.killDistance;
    for (auto i : range(current.influencePoints.size())){
        double d = distance(current._end, current.influencePoints[i]);

        if (d <= killDistance){
            current.influencePoints.erase(next(current.influencePoints.begin(), i));
            attractionArray.erase(next(attractionArray.begin(), current.indexesInfluence[i]));
        }
    }
}


// quicksort code from:
// https://www.geeksforgeeks.org/cpp-program-for-quicksort/ adapted to sort
// influence points by distance from reference tree node
template <typename T>
void swap(vector<T>& array, int index1, int index2) {
  auto a        = array[index1];
  array[index1] = array[index2];
  array[index2] = a;
}


auto partition_vec3f(vector<vec3f>& arrVec3f, int start, int end) {
  float pivot = arrVec3f[start].y;
  int   count = 0;
  for (int i = start + 1; i <= end; i++) {
        if (arrVec3f[i].y <= pivot) count++;
  }
  // Giving pivot element its correct position
  int pivotIndex = start + count;
  swap(arrVec3f, pivotIndex, start);
  // Sorting left and right parts of the pivot element
  int i = start, j = end;
  while (i < pivotIndex && j > pivotIndex) {
        while (arrVec3f[i].y <= pivot) { i++;}
        while (arrVec3f[j].y > pivot) { j--; }

        if (i < pivotIndex && j > pivotIndex) {
            swap(arrVec3f, i++, j--);
        }
  }
  return pivotIndex;
}


auto quicksort_vec3f(vector<vec3f>& vec, int start, int end) {
  if (start >= end) return;
  // partitioning the array
  auto p = partition_vec3f(vec, start, end);
  quicksort_vec3f(vec, start, p - 1);  // Sorting the left part
  quicksort_vec3f(vec, p + 1, end);    // Sorting the right part
}


//auto partition_InfluenceSet(influenceData& influence, int start, int end) {
//  // unpack data
//  auto influencePoints = influence.influencePointsArray;
//  auto indexes         = influence.influencePointsIndexes;
//  auto distancesArray  = influence.distances;
//
//  float pivot = distancesArray[start];
//  int   count = 0;
//  // arr == distancesArray
//  for (int i = start + 1; i <= end; i++) {
//    if (distancesArray[i] <= pivot) count++;
//  }
//  // Giving pivot element its correct position
//  int pivotIndex = start + count;
//  swap(distancesArray, pivotIndex, start);
//  // Sorting left and right parts of the pivot element
//  int i = start, j = end;
//  while (i < pivotIndex && j > pivotIndex) {
//    while (distancesArray[i] <= pivot) { i++; }
//    while (distancesArray[j] > pivot) { j--; }
//
//    if (i < pivotIndex && j > pivotIndex) {
//      swap(distancesArray, i++, j--);
//    }
//  }
//  return pivotIndex;
//}
//
//
//auto quicksort_InfluenceSet(influenceData& influence, int start, int end) {
//  if (start >= end) return;
//  // partitioning the array
//  auto p = partition_InfluenceSet(influence, start, end);
//  quicksort_InfluenceSet(influence, start, p - 1);  // Sorting the left part
//  quicksort_InfluenceSet(influence, p + 1, end);    // Sorting the right part
//} // end Quicksort implementation
}  // namespace yocto

