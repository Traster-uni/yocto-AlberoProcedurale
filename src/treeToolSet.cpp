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

typedef struct treeNode {
  // singolo nodo dell'albero
  vec3f     pos;        // coordinate spaziali del nodo
  treeNode* father_ptr;  // puntatore al treeNode padre
  int   numBranches;  // definisce il numero di figli che può ancora generare
  bool  fertile;      // definisce se il nodo è fertile o meno
  // direzione implementabile come prodotto scalare di un vettore direzione dato
  // dalla combinazione di due angoli randomici e la lunghezza del vettore
} treeNode;


typedef struct attractionPoints {
  // Conteiner per la corona di punti di attrazione
  vector<vec3f> attractionPointsArray;  // vettore di attraction points in uno spazio
  int attractionPointsSize;   // dimensione del vettore di punti di attrazione
  string treeCrownShape;  // directoy del modello blender per la forma della chioma
  string distribution;     // tipo di distribuzione dei punti di attrazione
  float  radiusInfluence;  // raggio di influenza di ogni punto di attrazione
  float  killDistance;     // raggio di eliminazione dei punti di influenza
  shape_data* shapeIndex; // Indice riferito al json della scena che indica la posizione del modello.
} attractionPoints;


typedef struct treeNodesContainer {
  // container per i nodi dell'albero
  vector<treeNode> treeNodesArray;  // vettore di stuct per i nodi dell'albero
  float            distance;        // distaza fra un punto ed il suo successore
  int              maxBranches;               // numero massimo di figli che un branch può generare
  shape_data       shape;           // caricati in tempo di esecuzione: il vero modello
  texture_data     texture;         // caricati in tempo di esecuzione: la vera texture
} treeNodesContainer;


struct influenceData {
  // sottoinsieme di punti di influenza per un treeNode
  vec3f         reference_node; // tree node a cui l'inflence set fa riferimento
  vector<vec3f> influencePointsArray; // l'array di influence points
  vector<int>   influencePointsIndexes; // array di indici legati agli attraction points
  vector<float> distances; // distanze degli influencePointsArray
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

vector<vec3f> populateSphere(int num_points, int seed) {
  auto rng = make_rng(seed); // seed the generator
  vector<vec3f> rdmPoints_into_sphere;
  for (auto i : range(num_points)){
        rdmPoints_into_sphere.push_back(sample_sphere(rand2f(rng)));
  }
  return rdmPoints_into_sphere;
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
auto findInfluenceSet(vec3f current_node, attractionPoints& treeCrown, influenceData& data) {
  auto radiusInfluence       = treeCrown.radiusInfluence;
  auto attractionPointsArray = treeCrown.attractionPointsArray;

  data = {current_node};

  int i = 0;
  for (auto ap : attractionPointsArray) {
    auto d = distance_squared(ap, current_node);

    if (d < radiusInfluence) {
      data.influencePointsArray.push_back(ap);
      data.influencePointsIndexes.push_back(i);
      data.distances.push_back(d);
    }
    i++;
  }
  return data;
}


auto findDirection_random(treeNodesContainer& treeNodes, influenceData& influenceSubset, float radius){
    /*
     * Finds the direction of the new tree node given the closest influence point,
     * then it scrambles its direction using the sample_sphere() function of yocto
     * TODO: DA COMPLETARE SE NECESSARIO
     */
    auto treeNode = influenceSubset.reference_node;
    auto influenceArray = influenceSubset.influencePointsArray;
    float min_distance = radius;
    vec3f min_attractionPoint = {0,0,0};
    for(int i = 0; i < influenceArray.size(); i++){
        if (influenceSubset.distances[i] < min_distance){
            min_distance = influenceSubset.distances[i];
            min_attractionPoint = influenceArray[i];
        }
    }
    vec3f p_d = {min_attractionPoint.x - treeNode.x, min_attractionPoint.y - treeNode.y, min_attractionPoint.z - treeNode.z};
    return;
}


auto findDirection_normalized(treeNodesContainer& nodesContainer, influenceData& influenceSubset){
    /*
     * Finds the direction of the new tree node given n closest influence points,
     * then normalizes the direction given the euler distances from the tree node
     *
     * TODO: auto rsu = sample_sphere(rand2f(rng));     // random spherical direction
     */
    auto treeNode = influenceSubset.reference_node;
    auto influenceArray = influenceSubset.influencePointsArray;
    random_device gen;
    mt19937 rng(gen());
    uniform_int_distribution<int> randomInt(0, influenceArray.size());
    auto d = influenceSubset.distances[randomInt(gen)];
    vector<vec3f> pointsArray;
    vector<vec3f> vectorArray;
    for(int i = 0; i < influenceArray.size(); i++){
        if (influenceSubset.distances[i] < d){
            pointsArray.push_back(influenceArray[i]);
            vectorArray.push_back(vec3f {influenceArray[i].x - treeNode.x, influenceArray[i].y - treeNode.y, influenceArray[i].z - treeNode.z});
        }
    }
    vec3f sum_point_weight = {0,0,0};
    float sum_distance_weight = 0;
    for (int i = 0; i < pointsArray.size(); i++){
        float sum_dis = d - influenceSubset.distances[i];
        //sum numeratore
        sum_point_weight.x += influenceArray[i].x * sum_dis;
        sum_point_weight.y += influenceArray[i].y * sum_dis;
        sum_point_weight.z += influenceArray[i].z * sum_dis;
        //sum denominatore
        sum_distance_weight += sum_dis;
    }
    vec3f final_norm = {sum_point_weight.x / sum_distance_weight,sum_point_weight.y / sum_distance_weight,sum_point_weight.z / sum_distance_weight};
    return final_norm;
}

auto insertNewTreeNode(vec3f direction, scene_data scene, treeNodesContainer treeNodes, treeNode father){
    float distance_insertion = treeNodes.distance;
} // TODO: piazza il tree node a distanza d dal nuovo nodo in nodesContainer


auto deleteAttractionPoints(influenceData& influenceSet, attractionPoints& treeCrown){
    auto influenceArray = influenceSet.influencePointsArray;
    auto attractionArray = treeCrown.attractionPointsArray;
    auto killDistance = treeCrown.killDistance;
    auto treeNode = influenceSet.reference_node;
    for (int i = 0; i < influenceArray.size(); i++){
        auto d = distance_squared(treeNode, influenceArray[i]);
        if (d <= killDistance){
            influenceArray.erase(next(influenceArray.begin(), i));
            attractionArray.erase(next(attractionArray.begin(), i));
        }
    }
}


// quicksort code from:
// https://www.geeksforgeeks.org/cpp-program-for-quicksort/ adapted to sort
// influence points by distance from reference tree node
void swap(vector<float>& array, int index1, int index2) {
  auto a        = array[index1];
  array[index1] = array[index2];
  array[index2] = a;
}


auto partition_InfluenceSet(influenceData& influence, int start, int end) {
  // unpack data
  auto influencePoints = influence.influencePointsArray;
  auto indexes         = influence.influencePointsIndexes;
  auto distancesArray  = influence.distances;

  float pivot = distancesArray[start];
  int   count = 0;
  // arr == distancesArray
  for (int i = start + 1; i <= end; i++) {
    if (distancesArray[i] <= pivot) count++;
  }
  // Giving pivot element its correct position
  int pivotIndex = start + count;
  swap(distancesArray, pivotIndex, start);
  // Sorting left and right parts of the pivot element
  int i = start, j = end;
  while (i < pivotIndex && j > pivotIndex) {
    while (distancesArray[i] <= pivot) {
      i++;
    }
    while (distancesArray[j] > pivot) {
      j--;
    }

    if (i < pivotIndex && j > pivotIndex) {
      swap(distancesArray, i++, j--);
    }
  }
  return pivotIndex;
}


auto quicksort_InfluenceSet(influenceData& influence, int start, int end) {
  if (start >= end) return;
  // partitioning the array
  auto p = partition_InfluenceSet(influence, start, end);
  quicksort_InfluenceSet(influence, start, p - 1);  // Sorting the left part
  quicksort_InfluenceSet(influence, p + 1, end);    // Sorting the right part
} // end Quicksort implementation
}  // namespace yocto

