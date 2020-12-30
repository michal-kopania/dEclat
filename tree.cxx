//
// Created by mic on 12/20/20.
//

#include <iostream>
#include "tree.hpp"
#include <fstream>

using namespace std;

void tree::add(node *current_node, node *parent_node)
{
    current_node->parent = parent_node;
    if(parent_node != nullptr) {
        parent_node->children.push_back(current_node);
        current_node->level = parent_node->level + 1;
        if(current_node->level > this->max_level) {
            this->max_level = current_node->level;
        }
    }
}

void tree::print(node *pNode)
{
    //visited[pNode] = true;
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        (*it)->print();
        //  auto search = visited.find(*it);
        //if(search == visited.end()) {
            this->print((*it));
        //}
    }
}


void tree::print()
{
    //visited.clear();
    print(root);
}

void tree::print_frequent_itemset(const std::string &file)
{
    string ascendant = "";
    ofstream myfile;
    if(file != "") {
        myfile.open(file, fstream::out | fstream::trunc);
        if(!myfile) {
            cout << "Cannot open file: " << file << endl << "Results will not be saved and be printed at stdio instead"
                 << endl;
            myfile.close();
        } else {
            myfile << "length, sup, discovered_frequent_itemset" << endl;
        }
    }

    this->print_frequent_itemset(this->root, ascendant, myfile);
    if(myfile.is_open()) {
        myfile.close();
    }
}

void tree::print_frequent_itemset(node *pNode, std::string all_ascendats, ofstream &file)
{
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if(pNode->level == this->max_level) {
            ++number_of_items_of_greatest_cardinality;
        }
        ////format: length, sup, discovered_frequent_itemset
        string parent = all_ascendats;
        if((*it)->level > 1) {
            parent += ", ";
        }
        parent += to_string((*it)->element);
        if(file.is_open()) {
            file << (*it)->level << ", " << (*it)->support << ", {" << parent << "}" << endl;
        } else {
            cout << (*it)->level << ", " << (*it)->support << ", {" << parent << "}" << endl;
        }
        this->print_frequent_itemset((*it), parent, file);
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
