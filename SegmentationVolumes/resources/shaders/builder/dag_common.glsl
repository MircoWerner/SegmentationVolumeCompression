/**
 * @param dagRoot [ root node index | root node index | ... ] - each root node index points to a node in the dag
 * @param dagRootCount root count
 * @param dag [ all dags of level 0 | all dags of level 1 | ... | all dags of level N ] - level 0 contains leaf nodes
 * @param dagCount total dag node count (sum of all level node counts)
 * @param dagLevels [ index and count of level 0 | index and count of level 1 | ... | index and count of level N ] - description of the dag levels
 */

#extension GL_EXT_shader_explicit_arithmetic_types_int64: require

#define DAG_INVALID_POINTER 0xFFFFFFFFu

struct DAGLevel {
    uint index;
    uint count;
};

struct DAGNode {
    uint child0;
    uint child1;
    uint child2;
    uint child3;
    uint child4;
    uint child5;
    uint child6;
    uint child7;
};

//uint hashF(in uint x) {
//    x = ((x >> 16) ^ x) * 0x45d9f3b;
//    x = ((x >> 16) ^ x) * 0x45d9f3b;
//    x = (x >> 16) ^ x;
//    return x;
//}
//
//void hash_combine(inout uint seed, in uint v) {
//    seed ^= hashF(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//}
//
//uint hash(DAGNode node) {
//    uint seed = hashF(node.child0);
//    hash_combine(seed, node.child1);
//    hash_combine(seed, node.child2);
//    hash_combine(seed, node.child3);
//    hash_combine(seed, node.child4);
//    hash_combine(seed, node.child5);
//    hash_combine(seed, node.child6);
//    hash_combine(seed, node.child7);
//    return seed;
//}

uint64_t hashF(in uint64_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

void hash_combine(inout uint64_t seed, in uint64_t v) {
    seed ^= hashF(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

uint64_t hash(DAGNode node) {
    uint64_t seed = hashF(node.child0);
    hash_combine(seed, node.child1);
    hash_combine(seed, node.child2);
    hash_combine(seed, node.child3);
    hash_combine(seed, node.child4);
    hash_combine(seed, node.child5);
    hash_combine(seed, node.child6);
    hash_combine(seed, node.child7);
    return seed;
}

//bool dag_common_compare(DAGNode a, DAGNode b) {
//    uint seed = hash(a);
//    uint otherSeed = hash(b);
//    return seed < otherSeed;
//}

bool dag_common_equals(DAGNode a, DAGNode b) {
    return a.child0 == b.child0 && a.child1 == b.child1 && a.child2 == b.child2 && a.child3 == b.child3 && a.child4 == b.child4 && a.child5 == b.child5 && a.child6 == b.child6 && a.child7 == b.child7;
}