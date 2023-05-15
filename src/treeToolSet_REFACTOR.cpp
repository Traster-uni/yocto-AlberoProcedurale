#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <cmath>

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
  int ARRAY_SIZE;
  vec3f* attractionPointsPtr;  // vettore di attraction points in uno spazio
  string treeCrownShape;  // directoy del modello blender per la forma della chioma
  float  radiusInfluence;  // raggio di influenza di ogni punto di attrazione
  float  killDistance;     // raggio di eliminazione dei punti di influenza
} attractionPoints;


typedef struct Branch {
  // singolo nodo dell'albero
  vec3f     _start;        // coordinate spaziali del nodo
  vec3f     _end;
  vec3f     _direction;
  float     _length;
  Branch*   father_ptr{};  // puntatore al branch padre
  vector<Branch> children;
  vector<vec3f*> influencePoints;
  int   maxBranches; // definisce il numero massimo di figli generabili
  int   minBranches; // definisce il numero minimo di figli da generare
  int   depth;
  float trunkDiameter; // Diametro del tronco sul punto.
  bool  fertile;      // definisce se il nodo è fertile o meno
  bool  branch;       // it's a branch
} Branch;

template <typename T>
T maxInVector (vector<T> a){
  auto max = a[0];
  for (auto elem : a){
    if (elem > max){
      max = elem;
    }
  }
  return max;
}

// UTIL FUNCTIONS
vec3f sample_sphere(mt19937& generator){
  uniform_real_distribution<float> floatDistribution(-1, 1);

  vec3f rvc3f = {floatDistribution(generator), floatDistribution(generator), floatDistribution(generator)};
  double mag = sqrt(sum(vec3f{ rvc3f.x*rvc3f.x, rvc3f.y*rvc3f.y, rvc3f.z*rvc3f.z}));
  float c = ::cbrt(floatDistribution(generator));
  return rvc3f * c;
}


int randomSeed(bool random, const int& interval_start, const int& interval_end, mt19937& generator) {
    if (!random) {
        return 58380;
    } else {
        uniform_int_distribution<int> intDistribution(interval_start, interval_end);
        return intDistribution(generator);
    }
}


vec3f computeAngles(vec3f origin, vec3f direction) {
  vec3f rotation;
  if (origin != vec3f{0,0,0}){
    // plane y-z --> x rotation --> PITCH
    if ((origin.y != 0 || origin.z != 0) && (direction.y != 0 || direction.z != 0)) {
      cout << " ^ YZ PLANE --> X ROTATION --> PITCH" << endl;
      auto num_yz =  dot(vec2f{origin.y, origin.z}, vec2f{direction.y, direction.z});
      cout << "   num_yz= " << num_yz << endl;
      auto modOrigin_yz = sqrt(sqr(origin.y) + sqr(origin.z));
      cout << "   modOrigin_yz= " << modOrigin_yz << endl;
      auto modDir_yz    = sqrt(sqr(direction.y) + sqr(direction.z));
      cout << "   modDir_YZ= " << modDir_yz << endl;
      auto denum_yz     = modOrigin_yz * modDir_yz;
      cout << "   denum_yz= " << denum_yz << endl;
      rotation.x        = ::acos(num_yz / denum_yz);
      cout << "   rotation.x= " << rotation.x << endl;
    } else { rotation.x = 0; }

    // plane z-x --> y rotation --> YAW
    if ((origin.x != 0 || origin.z != 0) && (direction.x != 0 || direction.z != 0)){
      cout << " <-> ZX PLANE --> Y ROTATION --> YAW" << endl;
      auto num_zx = dot(vec2f{origin.z, origin.x}, vec2f{direction.z, direction.x});
      cout << "     num_zx= " << num_zx << endl;
      auto modOrigin_zx = sqrt(sqr(origin.z) + sqr(origin.x));
      cout << "     modOrigin_zx= " << modOrigin_zx << endl;
      auto modDir_zx    = sqrt(sqr(direction.z) + sqr(direction.x));
      cout << "     modDir_zx= " << modDir_zx << endl;
      auto denum_zx     = modOrigin_zx * modDir_zx;
      cout << "     denum_zx= " << denum_zx << endl;
      rotation.y        = ::acos(num_zx / denum_zx);
      cout << "     rotation.y= " << rotation.y << endl;
    } else { rotation.y = 0; }

    // plane x-y --> z rotation --> ROLL
    if ((origin.x != 0 || origin.y != 0) && (direction.x != 0 || direction.y != 0)){
      cout << " (.) XY PLANE --> Z ROTATION --> ROLL" << endl;
      auto num_xy = dot(vec2f{origin.x, origin.y}, vec2f{direction.x, direction.y});
      cout << "     num_xy= " << num_xy << endl;
      auto modOrigin_xy = sqrt(sqr(origin.x) + sqr(origin.y));
      cout << "     modOrigin_xy= " << modOrigin_xy << endl;
      auto modDir_xy    = sqrt(sqr(direction.x) + sqr(direction.y));
      cout << "     modDir_xy= " << modDir_xy << endl;
      auto denum_xy     = modOrigin_xy * modDir_xy;
      cout << "     denum_xy= " << denum_xy << endl;
      rotation.z        = ::acos(num_xy / denum_xy);
      cout << "     rotation.z= " << rotation.z << endl;
    } else { rotation.z = 0; }
    return rotation;
  }
  return direction;
}
//TODO: AGGIORNA treeToolSet.cpp con le modifiche alle funzioni apportate in questo file;
bool populateSphere(vec3f array[], int const& ARRAY_SIZE, int num_points, mt19937& generator) {
  if (num_points <= ARRAY_SIZE){
    for (int i = 0; i < ARRAY_SIZE; i++){
      array[i] = sample_sphere(generator);
//      cout << "sampleing: " << array[i].x << ", " << array[i].y << ", " << array[i].z << ", " << endl;
    }
    return true;
  }
  return false;
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
  return 0.0f;
}


// SPATIAL COLONIZATION FUNCTIONS
// Simple checks

bool checkHeight(Branch* current, vec3f minVec) { return current->_end.y < minVec.y; }


bool isFertile(Branch *current){
  if (current->depth == 0){
    return false;
  }else if (current->children.size() < current->maxBranches && !current->influencePoints.empty()){
    return true;
  }
  return false;
}


bool canBranch(mt19937& generator){
  int rnd = randomSeed(true, 0, 100, generator);
  if (rnd <= 20){
    return true;
  }
  return false;
}


// Structs modifications

void findInfluenceSet(Branch *current, attractionPoints& treeCrown) {
  double radiusInfluence = treeCrown.radiusInfluence;
  for (int i = 0; i < treeCrown.ARRAY_SIZE; i++) {
    if (treeCrown.attractionPointsPtr[i] != vec3f{0,0,0}) {
      double d = distance(current->_end, treeCrown.attractionPointsPtr[i]);
      if (d <= radiusInfluence) {
        current->influencePoints.push_back(&treeCrown.attractionPointsPtr[i]);
      }
    }
  }
}


vec3f computeDirection(Branch* fatherBranch, const int& seed){
  rng_state rng = make_rng(seed);
  vec3f newDir = {0, 0, 0};
  vec3f num;
  float denom;
  for (auto ip : fatherBranch->influencePoints) {
    num = *ip - fatherBranch->_end;
    denom  = length(num);
    newDir += vec3f{num.x / denom, num.y / denom, num.z / denom};
  }
  newDir /= fatherBranch->influencePoints.size();
  auto newDir_norm = sqrt(dot(newDir, newDir));
  return (newDir / newDir_norm) + rand3f(rng);
}


vec3f rndDirection(Branch& fatherBranch, const int &seed){
  rng_state rng = make_rng(seed);
  return fatherBranch._direction += rand3f(rng);
}


Branch *growChild(Branch *fatherBranch, vec3f direction, mt19937& generator){
  auto newBranch         = new Branch ;
  newBranch->_start      = fatherBranch->_end;
  newBranch->_direction  = direction * fatherBranch->_length;
  newBranch->_end        = newBranch->_direction + fatherBranch->_end;
  newBranch->_length     = fatherBranch->_length;
  newBranch->father_ptr  = fatherBranch;
  newBranch->fertile     = false;
  newBranch->maxBranches = fatherBranch->maxBranches;
  newBranch->minBranches = fatherBranch->minBranches;
  newBranch->depth       = fatherBranch->depth-1;
  newBranch->branch      = canBranch(generator);
  fatherBranch->children.push_back(*newBranch);
  return &fatherBranch->children[fatherBranch->children.size()-1];
}


void clearInfluenceSet(Branch *branch){
    branch->influencePoints.clear();
}


void deleteAttractionPoints(Branch* current, attractionPoints& treeCrown, const vec3f& floorPos){
  auto killDistance = treeCrown.killDistance;

  for ( int i = current->influencePoints.size()-1; i >= 0; i-- ){
//    cout << "284: " << current.influencePoints.size() << endl;
    double d = length(current->_end - *(current->influencePoints[i])); //TODO: errore
    //  STAMPA CONTENUTO
    if (d <= killDistance){
      delete current->influencePoints[i];
    }
  }
  current->influencePoints.erase(current->influencePoints.begin(), current->influencePoints.end());
}
//
void bubbleSort(vec3f x[], int size) {
  //from https://cplusplus.com/forum/beginner/12731/
  bool exchanges;
  do {
    exchanges = false;  // assume no exchanges
    for (int i=0; i<size-1; i++) {
      if (x[i].y > x[i+1].y) {
        auto temp = x[i]; x[i] = x[i+1]; x[i+1] = temp;
        exchanges = true;  // after exchange, must look again
      }
    }
  } while (exchanges);
}

}  // namespace yocto

