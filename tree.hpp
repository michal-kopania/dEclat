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

    virtual ~node()
    {
        for(auto it = children.begin(); it != children.end(); ++it) {
            delete *it;
        }
    }

};

struct tree
{
    node *root;
    std::unordered_map<node *, bool> visited;

    void add(node *current_node, node *parent_node);

    void print();

    //format: length, sup, discovered_frequent_itemset
    void print_frequent_itemset(const std::string &file);

    virtual ~tree()
    {
        delete root;
    }

private:
    void print(node *pNode);

    void print_frequent_itemset(node *pNode, std::string all_ascendats, std::ofstream &file);
};


#endif //DECLAT_TREE_HPP
