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
    std::vector<node *> children; //List of children
    unsigned int element = 0; //Item id
    unsigned int support = 0; //Support number
    std::set<unsigned int> diff_set; //set of transactions ids
    unsigned int level = 0; //On which level of a tree node is
    //std::string name;

    void print();

    ~node()
    {
        for(auto it = children.begin(); it != children.end(); ++it) {
            delete *it;
        }
    }

};

struct tree
{
    node *root; //Root node
    unsigned int max_level = 0; //How deep is a tree
    unsigned int number_of_items_of_greatest_cardinality = 0; //the number of items in a discovered frequent itemset of the greatest cardinality

    //std::unordered_map<node *, bool> visited; //Needed for graph not needed for tree

    void add(node *current_node, node *parent_node);

    void print();

    //format: length, sup, discovered_frequent_itemset
    void print_frequent_itemset(const std::string &file);

    ~tree()
    {
        delete root;
    }

private:
    void print(node *pNode);

    void print_frequent_itemset(node *pNode, std::string all_ascendats, std::ofstream &file);
};


#endif //DECLAT_TREE_HPP
