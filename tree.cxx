//
// Created by mic on 12/20/20.
//

#include <iostream>
#include "tree.hpp"

using namespace std;

void tree::add(node *current_node, node *parent_node)
{
    current_node->parent = parent_node;
    if(parent_node != nullptr) {
        parent_node->children.push_back(current_node);
        current_node->level = parent_node->level + 1;
    }
}

void node::print()
{
    cout << "Level: " << this->level << " element: " << this->element << " sup: " << this->support << endl << "D({";
    for(auto it = this->diff_set.begin(); it != this->diff_set.end(); ++it) {
        cout << *it << ", ";
    }
    cout << "})" << endl;
}
