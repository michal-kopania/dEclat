//
// Created by mic on 12/20/20.
//

#ifndef DECLAT_TREE_HPP
#define DECLAT_TREE_HPP

#include <string>
#include <set>
#include <vector>
#include <unordered_map>

struct node
{
    node *parent = nullptr;
    std::vector<node *> children;
    unsigned long element = 0;
    unsigned int support = 0;
    std::set<unsigned int> diff_set; //transactions ids
    unsigned int level = 0;
    //std::string name;

    void print();
};

struct tree
{
    node *root;
    std::unordered_map<node *, bool> visited;

    void add(node *current_node, node *parent_node);

    void print(node *pNode);
};


#endif //DECLAT_TREE_HPP
