//
// Created by mic on 12/20/20.
//

#ifndef DECLAT_TREE_HPP
#define DECLAT_TREE_HPP

#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>

struct node
{
    node *parent = nullptr; //Pointer to parent

    /*
     * @brief List of children.
     * Children of root are singleton items from vertical_representation
     * Down the tree children are added as described https://arxiv.org/pdf/1901.07773v1.pdf
     */
    std::vector<node *> children;

    unsigned int element = 0; //Item id

    unsigned int support = 0; //Support number

    std::set<unsigned int> diff_set; //set of diff transactions ids

    unsigned int level = 0; //On which level of a tree node is

    //bool visited = false;
    //std::string name;

    void print();

    ~node()
    {
        for(auto it = children.begin(); it != children.end(); ++it) {
            delete *it;
            *it = nullptr;
        }
    }

};

struct tree
{
    node *root; //Root node
    unsigned int max_level = 0; //How deep is a tree

    //std::unordered_map<node *, bool> visited; //Needed for graph not needed for tree

    void add(node *current_node, node *parent_node);

    void print(bool sorted);

    //format: length, sup, discovered_frequent_itemset
    void print_frequent_itemset(const std::string &file, bool sorted);

    void print_frequent_itemset(node *pNode, std::string all_ascendants, std::ofstream &file);

    void print_frequent_itemset_sorted(node *pNode, std::set<unsigned int> all_ascendants, std::ofstream &file);

    ~tree()
    {
        delete root;
    }

private:
    void print(node *pNode);

};


#endif //DECLAT_TREE_HPP
