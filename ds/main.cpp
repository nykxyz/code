#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <stack>
#include <unordered_map>
#include <list>
#include <set>

// 定义颜色常量用于命令行可视化
const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";

// 1. 单链表实现
template <typename T>
class LinkedList {
private:
    struct Node {
        T data;
        std::shared_ptr<Node> next;
        Node(T val) : data(val), next(nullptr) {}
    };
    std::shared_ptr<Node> head;
    size_t size;

public:
    LinkedList() : head(nullptr), size(0) {}

    void push_back(T val) {
        auto new_node = std::make_shared<Node>(val);
        if (!head) {
            head = new_node;
        } else {
            auto current = head;
            while (current->next) {
                current = current->next;
            }
            current->next = new_node;
        }
        size++;
    }

    void push_front(T val) {
        auto new_node = std::make_shared<Node>(val);
        new_node->next = head;
        head = new_node;
        size++;
    }

    bool remove(T val) {
        if (!head) return false;
        
        if (head->data == val) {
            head = head->next;
            size--;
            return true;
        }
        
        auto current = head;
        while (current->next && current->next->data != val) {
            current = current->next;
        }
        
        if (current->next) {
            current->next = current->next->next;
            size--;
            return true;
        }
        
        return false;
    }

    void visualize() {
        std::cout << BLUE << "Linked List (size: " << size << "): " << RESET;
        auto current = head;
        while (current) {
            std::cout << current->data;
            if (current->next) {
                std::cout << " -> ";
            }
            current = current->next;
        }
        std::cout << std::endl;
    }

    size_t getSize() const {
        return size;
    }
};

// 2. 栈实现
template <typename T>
class Stack {
private:
    std::vector<T> elements;

public:
    void push(T val) {
        elements.push_back(val);
    }

    bool pop() {
        if (empty()) return false;
        elements.pop_back();
        return true;
    }

    T top() const {
        if (empty()) throw std::runtime_error("Stack is empty");
        return elements.back();
    }

    bool empty() const {
        return elements.empty();
    }

    size_t size() const {
        return elements.size();
    }

    void visualize() {
        std::cout << RED << "Stack (size: " << size() << "): " << RESET;
        for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
            std::cout << "| " << *it << " |\n";
        }
        std::cout << "+---+" << std::endl;
    }
};

// 3. 队列实现
template <typename T>
class Queue {
private:
    std::deque<T> elements;

public:
    void enqueue(T val) {
        elements.push_back(val);
    }

    bool dequeue() {
        if (empty()) return false;
        elements.pop_front();
        return true;
    }

    T front() const {
        if (empty()) throw std::runtime_error("Queue is empty");
        return elements.front();
    }

    bool empty() const {
        return elements.empty();
    }

    size_t size() const {
        return elements.size();
    }

    void visualize() {
        std::cout << GREEN << "Queue (size: " << size() << "): " << RESET;
        std::cout << "<- [ ";
        for (size_t i = 0; i < elements.size(); ++i) {
            std::cout << elements[i];
            if (i < elements.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << " ]" << std::endl;
    }
};

// 4. 二叉树实现
template <typename T>
class BinaryTree {
private:
    struct Node {
        T data;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        Node(T val) : data(val), left(nullptr), right(nullptr) {}
    };
    std::shared_ptr<Node> root;

    void insertRecursive(std::shared_ptr<Node>& node, T val) {
        if (!node) {
            node = std::make_shared<Node>(val);
            return;
        }
        if (val < node->data) {
            insertRecursive(node->left, val);
        } else {
            insertRecursive(node->right, val);
        }
    }

    void inorderRecursive(const std::shared_ptr<Node>& node) {
        if (node) {
            inorderRecursive(node->left);
            std::cout << node->data << " ";
            inorderRecursive(node->right);
        }
    }

    void visualizeRecursive(const std::shared_ptr<Node>& node, int level) {
        if (!node) return;
        
        visualizeRecursive(node->right, level + 1);
        
        for (int i = 0; i < level; ++i) {
            std::cout << "    ";
        }
        std::cout << MAGENTA << node->data << RESET << std::endl;
        
        visualizeRecursive(node->left, level + 1);
    }

public:
    BinaryTree() : root(nullptr) {}

    void insert(T val) {
        insertRecursive(root, val);
    }

    void inorder() {
        inorderRecursive(root);
        std::cout << std::endl;
    }

    void visualize() {
        std::cout << MAGENTA << "Binary Tree Visualization (rotated 90 degrees):" << RESET << std::endl;
        visualizeRecursive(root, 0);
    }
};

// 5. 图实现
template <typename T>
class Graph {
private:
    std::unordered_map<T, std::list<T>> adjList;

public:
    void addVertex(T vertex) {
        if (adjList.find(vertex) == adjList.end()) {
            adjList[vertex] = std::list<T>();
        }
    }

    void addEdge(T from, T to) {
        addVertex(from);
        addVertex(to);
        adjList[from].push_back(to);
    }

    void BFS(T start) {
        std::unordered_map<T, bool> visited;
        std::queue<T> q;
        
        q.push(start);
        visited[start] = true;
        
        std::cout << "BFS starting from " << start << ": ";
        
        while (!q.empty()) {
            T current = q.front();
            q.pop();
            std::cout << current << " ";
            
            for (const auto& neighbor : adjList[current]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    q.push(neighbor);
                }
            }
        }
        std::cout << std::endl;
    }

    void visualize() {
        std::cout << CYAN << "Graph Visualization (adjacency list):" << RESET << std::endl;
        for (const auto& pair : adjList) {
            std::cout << pair.first << " -> ";
            for (const auto& neighbor : pair.second) {
                std::cout << neighbor << " ";
            }
            std::cout << std::endl;
        }
    }
};

// 6. 哈希表实现
template <typename K, typename V>
class HashTable {
private:
    std::vector<std::list<std::pair<K, V>>> table;
    size_t size;
    size_t capacity;
    
    size_t hash(const K& key) const {
        return std::hash<K>{}(key) % capacity;
    }

public:
    HashTable(size_t cap = 10) : capacity(cap), size(0) {
        table.resize(capacity);
    }

    void insert(const K& key, const V& value) {
        size_t index = hash(key);
        
        // 检查键是否已存在
        for (auto& pair : table[index]) {
            if (pair.first == key) {
                pair.second = value; // 更新值
                return;
            }
        }
        
        table[index].emplace_back(key, value);
        size++;
        
        // 简单的扩容逻辑
        if (size > capacity * 0.75) {
            resize();
        }
    }

    bool remove(const K& key) {
        size_t index = hash(key);
        
        for (auto it = table[index].begin(); it != table[index].end(); ++it) {
            if (it->first == key) {
                table[index].erase(it);
                size--;
                return true;
            }
        }
        
        return false;
    }

    V* get(const K& key) {
        size_t index = hash(key);
        
        for (auto& pair : table[index]) {
            if (pair.first == key) {
                return &pair.second;
            }
        }
        
        return nullptr;
    }

    void resize() {
        std::vector<std::list<std::pair<K, V>>> oldTable = table;
        capacity *= 2;
        table.clear();
        table.resize(capacity);
        size = 0;
        
        for (const auto& bucket : oldTable) {
            for (const auto& pair : bucket) {
                insert(pair.first, pair.second);
            }
        }
    }

    void visualize() {
        std::cout << YELLOW << "Hash Table Visualization (size: " << size \
                  << ", capacity: " << capacity << "):" << RESET << std::endl;
        
        for (size_t i = 0; i < capacity; ++i) {
            std::cout << "Bucket " << i << ": ";
            
            for (const auto& pair : table[i]) {
                std::cout << "[" << pair.first << ":" << pair.second << "] ";
            }
            
            std::cout << std::endl;
        }
    }
};

// 主函数，演示所有数据结构
int main() {
    std::cout << "======= C++ 数据结构可视化演示 =======\n" << std::endl;
    
    // 1. 演示链表
    std::cout << "1. 链表 (Linked List)\n";
    LinkedList<int> linkedList;
    for (int i = 1; i <= 5; ++i) {
        linkedList.push_back(i * 10);
    }
    linkedList.visualize();
    std::cout << "删除元素 30 后：" << std::endl;
    linkedList.remove(30);
    linkedList.visualize();
    std::cout << "在头部添加元素 5：" << std::endl;
    linkedList.push_front(5);
    linkedList.visualize();
    std::cout << std::endl;
    
    // 2. 演示栈
    std::cout << "2. 栈 (Stack)\n";
    Stack<std::string> stack;
    stack.push("数据1");
    stack.push("数据2");
    stack.push("数据3");
    stack.visualize();
    std::cout << "弹出一个元素后：" << std::endl;
    stack.pop();
    stack.visualize();
    std::cout << std::endl;
    
    // 3. 演示队列
    std::cout << "3. 队列 (Queue)\n";
    Queue<double> queue;
    queue.enqueue(1.1);
    queue.enqueue(2.2);
    queue.enqueue(3.3);
    queue.visualize();
    std::cout << "出队一个元素后：" << std::endl;
    queue.dequeue();
    queue.visualize();
    std::cout << std::endl;
    
    // 4. 演示二叉树
    std::cout << "4. 二叉树 (Binary Tree)\n";
    BinaryTree<int> tree;
    tree.insert(50);
    tree.insert(30);
    tree.insert(70);
    tree.insert(20);
    tree.insert(40);
    tree.insert(60);
    tree.insert(80);
    tree.visualize();
    std::cout << "中序遍历: ";
    tree.inorder();
    std::cout << std::endl;
    
    // 5. 演示图
    std::cout << "5. 图 (Graph)\n";
    Graph<char> graph;
    graph.addEdge('A', 'B');
    graph.addEdge('A', 'C');
    graph.addEdge('B', 'D');
    graph.addEdge('C', 'E');
    graph.addEdge('D', 'E');
    graph.addEdge('E', 'A');
    graph.visualize();
    graph.BFS('A');
    std::cout << std::endl;
    
    // 6. 演示哈希表
    std::cout << "6. 哈希表 (Hash Table)\n";
    HashTable<std::string, int> hashTable;
    hashTable.insert("张三", 25);
    hashTable.insert("李四", 30);
    hashTable.insert("王五", 35);
    hashTable.insert("赵六", 40);
    hashTable.visualize();
    
    // 查找元素
    if (auto* age = hashTable.get("李四")) {
        std::cout << "李四的年龄: " << *age << std::endl;
    }
    
    std::cout << "\n======= 演示结束 =======\n";
    
    return 0;
}