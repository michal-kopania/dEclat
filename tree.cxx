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

void tree::print(node *pNode)
{
    visited[pNode] = true;
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        (*it)->print();
        auto search = visited.find(*it);
        if(search == visited.end()) {
            this->print((*it));
        }
    }
}

void node::print()
{
    cout << this << " Level: " << this->level << " element: " << this->element << " parent: "
         << (this->parent == nullptr ? 0 : this->parent->element) << " [" << this->parent << "]" << " sup: "
         << this->support << endl << "D({";
    for(auto it = this->diff_set.begin(); it != this->diff_set.end(); ++it) {
        cout << *it << ", ";
    }
    cout << "})" << endl;
}
