//
// Created by mic on 12/20/20.
//

#include "tree.hpp"

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


void tree::print(bool sorted)
{
    //visited.clear();
    this->print(root);
}

void tree::print_frequent_itemset(const std::string &file, bool sorted)
{
    string ascendant = "";
    ofstream myfile;
    if(file != "") {
        myfile.open(file, fstream::out | fstream::app);
        if(!myfile) {
            cout << "Cannot open file: " << file << endl << "Results will not be saved and be printed at stdio instead"
                 << endl;
            myfile.close();
        } else {
            //myfile << "length\tsup\tdiscovered_frequent_itemset" << endl;
        }
    }
    if(sorted) {
        std::set<unsigned int> set_ascendant;
        this->print_frequent_itemset_sorted(this->root, set_ascendant, myfile);
    } else {
        this->print_frequent_itemset(this->root, ascendant, myfile);
    }
    if(myfile.is_open()) {
        myfile.close();
    }
}

void tree::print_frequent_itemset(node *pNode, std::string all_ascendants, ofstream &file)
{
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it)->level == this->max_level) {
            ++number_of_items_of_greatest_cardinality;
        }
        ////format: length, sup, discovered_frequent_itemset
        string parent = all_ascendants;
        if((*it)->level > 1) {
            parent += ", ";
        }
        parent += to_string((*it)->element);
        if(file.is_open()) {
            file << (*it)->level << "\t" << (*it)->support << "\t" << parent << "" << endl;
        } else {
            cout << (*it)->level << "\t" << (*it)->support << "\t" << parent << "" << endl;
        }
        this->print_frequent_itemset((*it), parent, file);
    }
}

void tree::print_frequent_itemset_sorted(node *pNode, std::set<unsigned int> all_ascendants, ofstream &file)
{
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it)->level == this->max_level) {
            ++number_of_items_of_greatest_cardinality;
        }
        ////format: length, sup, discovered_frequent_itemset
        all_ascendants.insert((*it)->element);
        string parent = "";
        for(auto it = all_ascendants.begin(); it != all_ascendants.end(); ++it) {
            if(parent != "") {
                parent += ", ";
            }
            parent += to_string((*it));
        }
        if(file.is_open()) {
            file << (*it)->level << "\t" << (*it)->support << "\t" << parent << endl;
        } else {
            cout << (*it)->level << "\t" << (*it)->support << "\t" << parent << endl;
        }
        this->print_frequent_itemset_sorted((*it), all_ascendants, file);
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
