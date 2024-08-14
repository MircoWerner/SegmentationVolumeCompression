#pragma once

#include <algorithm>
#include <cstdint>
#include <execution>
#include <iostream>
#include <stdexcept>
#include <vector>

#define LOD_HEADER

namespace raven {
    class DAG {
    public:
        constexpr static uint32_t invalidPointer() { return 0xFFFFFFFF; }

        struct DAGNode {
            uint32_t child0 = invalidPointer();
            uint32_t child1 = invalidPointer();
            uint32_t child2 = invalidPointer();
            uint32_t child3 = invalidPointer();
            uint32_t child4 = invalidPointer();
            uint32_t child5 = invalidPointer();
            uint32_t child6 = invalidPointer();
            uint32_t child7 = invalidPointer();

            [[nodiscard]] static uint64_t hashF(uint64_t x) {
                x = ((x >> 16) ^ x) * 0x45d9f3b;
                x = ((x >> 16) ^ x) * 0x45d9f3b;
                x = (x >> 16) ^ x;
                return x;
            }

            static void hash_combine(uint64_t &seed, const uint64_t v) { seed ^= hashF(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2); }

            [[nodiscard]] uint64_t hash() const {
                uint64_t seed = hashF(child0);
                hash_combine(seed, child1);
                hash_combine(seed, child2);
                hash_combine(seed, child3);
                hash_combine(seed, child4);
                hash_combine(seed, child5);
                hash_combine(seed, child6);
                hash_combine(seed, child7);
                return seed;
            }

            [[nodiscard]] bool compare(const DAGNode &other) const {
                const uint64_t seed = hash();
                const uint64_t otherSeed = other.hash();
                return seed < otherSeed;
            }

            [[nodiscard]] bool equals(const DAGNode &other) const {
                return child0 == other.child0 && child1 == other.child1 && child2 == other.child2 && child3 == other.child3 && child4 == other.child4 && child5 == other.child5 && child6 == other.child6 && child7 == other.child7;
            }

            friend std::ostream &operator<<(std::ostream &stream, const DAGNode &node) {
                stream << "{ leaf= " << node.isLeaf() << ", solid=" << node.isSolid() << ", children=(" << node.child0 << ", " << node.child1
                       << ", " << node.child2 << ", " << node.child3 << ", " << node.child4 << ", " << node.child5 << ", " << node.child6 << ", " << node.child7 << ") }";
                return stream;
            }

            [[nodiscard]] bool isLeaf() const {
                return child0 == invalidPointer();
            }

            [[nodiscard]] bool isSolid() const {
                return isLeaf() && (child1 > 0 || child2 > 0); // higher level or solid/occupancy set
            }
        };

        struct DAGLevel {
            uint32_t index;
            uint32_t count;
        };

        typedef uint32_t DAGRoot;

        /**
     *
     * @param dagRoot [ root node index | root node index | ... ] - each root node index points to a node in the dag
     * @param dagRootCount root count
     * @param dag [ all dags of level 0 | all dags of level 1 | ... | all dags of level N ] - level 0 contains leaf nodes
     * @param dagCount total dag node count (sum of all level node counts)
     * @param dagLevels [ index and count of level 0 | index and count of level 1 | ... | index and count of level N ] - description of the dag levels
     */
        DAG(DAGRoot *dagRoot, const uint32_t dagRootCount, DAGNode *dag, const uint32_t dagCount, std::vector<DAGLevel> dagLevels)
            : m_dagRoot(dagRoot), m_dagRootCount(dagRootCount), m_dag(dag), m_dagCount(dagCount), m_dagLevels(std::move(dagLevels)) {
            if (m_dagLevels.empty()) {
                throw std::runtime_error("DAG must have at least one level.");
            }
            uint32_t index = 0;
            for (uint32_t i = 0; i < m_dagLevels.size(); i++) {
                const auto &level = m_dagLevels[i];
                if (level.index != index) {
                    throw std::runtime_error("DAG level index must be consecutive.");
                }
                if (level.count == 0) {
                    throw std::runtime_error("DAG level must have at least one node.");
                }
                index += level.count;
            }
            if (m_dagCount != index) {
                throw std::runtime_error("DAG node count must match sum of all level node counts.");
            }
        }

        void reduce(uint32_t *outDAGCount, std::vector<DAGLevel> &outDAGLevels) const {
            if (outDAGLevels.size() != m_dagLevels.size()) {
                throw std::runtime_error("DAG level count must match.");
            }

            uint32_t globalOffset = 0; // points to the first empty index in the dag array
            std::vector<uint32_t> indirectionList(m_dagCount);
            std::vector<uint32_t> indexList(m_dagCount);

            const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

            // store index pointer of all nodes in level before sorting
            for (uint32_t i = 0; i < m_dagCount; i++) {
                indexList[i] = i;
            }

            // bottom up reduction
            for (uint32_t l = 0; l < m_dagLevels.size(); l++) {
                // initialize output level
                auto &outLevel = outDAGLevels[l];
                outLevel.index = globalOffset;
                outLevel.count = 0;

                const auto &level = m_dagLevels[l];

                // sort level
                std::sort(std::execution::par_unseq, indexList.begin() + level.index, indexList.begin() + level.index + level.count, [this](const uint32_t &a, const uint32_t &b) { return m_dag[a].compare(m_dag[b]); });
                for (uint32_t i = level.index; i < level.index + level.count; i++) {
                    indirectionList[indexList[i]] = i;
                }
                for (uint32_t i = level.index; i < level.index + level.count; i++) {
                    while (indirectionList[i] != i) {
                        std::swap(m_dag[i], m_dag[indirectionList[i]]);
                        std::swap(indirectionList[i], indirectionList[indirectionList[i]]);
                    }
                }

                for (uint32_t i = 0; i < level.count; i++) {
                    auto node = m_dag[level.index + i];

                    // compact level
                    if (const bool unique = i == 0 || !node.equals(m_dag[level.index + i - 1])) {
                        m_dag[globalOffset] = node;
                        globalOffset++;
                        outLevel.count++;
                    }

                    // construct indirection list
                    indirectionList[indexList[level.index + i]] = globalOffset - 1;
                }

                if (l + 1 < m_dagLevels.size()) {
                    // update parent pointer in level l + 1
                    const auto &nextLevel = m_dagLevels[l + 1];
                    for (uint32_t i = 0; i < nextLevel.count; i++) {
                        auto &node = m_dag[nextLevel.index + i];
                        const bool isLeaf = node.isLeaf();
                        node.child0 = isLeaf ? node.child0 : indirectionList[node.child0];
                        node.child1 = isLeaf ? node.child1 : indirectionList[node.child1];
                        node.child2 = isLeaf ? node.child2 : indirectionList[node.child2];
                        node.child3 = isLeaf ? node.child3 : indirectionList[node.child3];
                        node.child4 = isLeaf ? node.child4 : indirectionList[node.child4];
                        node.child5 = isLeaf ? node.child5 : indirectionList[node.child5];
                        node.child6 = isLeaf ? node.child6 : indirectionList[node.child6];
                        node.child7 = isLeaf ? node.child7 : indirectionList[node.child7];
                    }
                } else {
                    // update root pointer
                    for (uint32_t i = 0; i < m_dagRootCount; i++) {
                        assert(m_dagRoot[i] < m_dagCount);
                        m_dagRoot[i] = m_dagRoot[i] == invalidPointer() ? invalidPointer() : indirectionList[m_dagRoot[i]];
                    }
                }

                std::cout << "[DAG] Reduced level " << l << "." << std::endl;
            }

            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            double cpuTime = (static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count()) * std::pow(10, -3));
            std::cout << "[DAG] " << cpuTime << "[ms]" << std::endl;

            uint32_t index = 0;
            for (uint32_t i = 0; i < outDAGLevels.size(); i++) {
                const auto &level = outDAGLevels[i];
                if (level.index != index) {
                    throw std::runtime_error("DAG level index must be consecutive.");
                }
                if (level.count == 0) {
                    throw std::runtime_error("DAG level must have at least one node.");
                }
                index += level.count;
            }
            *outDAGCount = index;

            std::cout << "[DAG] Reduced from " << m_dagCount << " to " << *outDAGCount << " nodes." << std::endl;
        }

        void verify() {
            std::vector visited(m_dagCount, false);

            for (uint32_t i = 0; i < m_dagRootCount; i++) {
                const auto &root = m_dagRoot[i];
                if (!verifyTraverse(visited, root, 4)) {
                    throw std::runtime_error("DAG traversal failed.");
                }
            }

            for (uint32_t i = 0; i < m_dagCount; i++) {
                if (!visited[i]) {
                    throw std::runtime_error("DAG node not reachable from any root. " + std::to_string(i) + "/" + std::to_string(m_dagCount));
                }
            }
        }

    private:
        DAGRoot *m_dagRoot;
        uint32_t m_dagRootCount;

        DAGNode *m_dag;
        uint32_t m_dagCount = 0;
        std::vector<DAGLevel> m_dagLevels;

        bool verifyTraverse(std::vector<bool> &visited, const uint32_t index, const uint32_t level) {
            if (index >= m_dagCount && index != invalidPointer()) {
                std::cerr << "DAG node index out of bounds." << std::endl;
                return false;
            }

            const auto &node = m_dag[index];

            visited[index] = true;

            if (node.isLeaf()) {
                return true;
            }

            return verifyTraverse(visited, node.child0, level - 1) && verifyTraverse(visited, node.child1, level - 1) && verifyTraverse(visited, node.child2, level - 1) && verifyTraverse(visited, node.child3, level - 1) &&
                   verifyTraverse(visited, node.child4, level - 1) && verifyTraverse(visited, node.child5, level - 1) && verifyTraverse(visited, node.child6, level - 1) && verifyTraverse(visited, node.child7, level - 1);
        }
    };
} // namespace raven