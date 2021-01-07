//
// Created by mic on 1/4/21.
//

#include "taxonomy_tree.hpp"

using namespace std;

extern unsigned int number_of_transactions;
extern unsigned min_sup;
extern void common(const std::set<unsigned int> &diff_set1, const std::set<unsigned int> &diff_set2,
                   std::set<unsigned int> &result);

#define DEBUG_LEVEL 0

void taxonomy_tree::add(unsigned int current_element, unsigned int parent_element)
{
    taxonomy_node *current_node, *parent_node;

    //Search for parent_element in roots
    auto search = roots.find(parent_element);
    if(search != roots.end()) {
        //Found in roots. Add to found root
        parent_node = search->second;
        auto search_current = roots.find(current_element);
        if(search_current != roots.end()) {
            //Current is in roots.
            parent_node->children.push_back(search_current->second);
            for(auto it = parent_node->children.begin(); it != parent_node->children.end(); ++it) {
                (*it)->parent = parent_node;
            }
            this->roots[current_element] = nullptr;
            roots.erase(search_current);
        } else {
            current_node = new taxonomy_node(current_element, parent_node);
            parent_node->children.push_back(current_node);
        }
    } else {
        //Parent is not in roots
        //Is my current_node in roots?
        auto search_current = roots.find(current_element);
        if(search_current != roots.end()) {
            //Yes.
            parent_node = new taxonomy_node(parent_element, nullptr);
            parent_node->children.push_back(search_current->second);
            for(auto it = parent_node->children.begin(); it != parent_node->children.end(); ++it) {
                (*it)->parent = parent_node;
            }
            //Add parent to roots
            this->roots[parent_element] = parent_node;
            //Remove from roots. Must assign null first
            this->roots[current_element] = nullptr;
            roots.erase(search_current);
            //auto e=roots.extract(search_current);

        } else {
            //Add pair current,parent add parent to roots
            parent_node = new taxonomy_node(parent_element, nullptr);
            current_node = new taxonomy_node(current_element, parent_node);
            parent_node->children.push_back(current_node);
            this->roots[parent_element] = parent_node;
        }
    }
}

void taxonomy_tree::print()
{
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        print((*it).second);
    }
}

void taxonomy_tree::print(taxonomy_node *pNode)
{
    pNode->print();
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it) != nullptr) {
            this->print((*it));
        }
    }
}


void taxonomy_tree::set_sup_from_vertical_representation(
        std::unordered_map<unsigned int, std::set<unsigned int> *> *vertical_representation)
{
    cout << "set_sup_from_vertical_representation()" << endl;
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        set_sup_from_vertical_representation((*it).second, vertical_representation);
    }
}

void taxonomy_tree::set_sup_from_vertical_representation(taxonomy_node *pNode,
                                                         std::unordered_map<unsigned int, std::set<unsigned int> *> *vertical_representation)
{
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it) != nullptr) {
            this->set_sup_from_vertical_representation((*it), vertical_representation);
        }
    }
    if(pNode->children.empty()) {
        //pNode is a leaf
        auto search = vertical_representation->find(pNode->element);
        if(search != vertical_representation->end()) {
            pNode->support = search->second->size();
            pNode->transaction_ids = *search->second; //copy. Its safer that way. Can be optimized
            //Copy from first level of tree
            //TODO: Either use diff_set or transaction_ids
            //I do not use diff_set
            /*
            for(auto it = tree.root->children.begin(); it != tree.root->children.end(); ++it) {
                if((*it)->element == pNode->element) {
                    pNode->diff_set = (*it)->diff_set; //copy.  Its safer. Can be optimized
                    break;
                }
            }
            */

        }
        //pNode->print();
    }
}

void taxonomy_tree::set_sup_from_children()
{
    cout << "set_sup_from_children()" << endl;
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        set_sup_from_children((*it).second);
    }
}

void taxonomy_tree::set_sup_from_children(taxonomy_node *pNode)
{
    //I calculating data starting from bottom. From leafs to root
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it) != nullptr) {
            this->set_sup_from_children((*it));
            //Child has calculated sup
            if(!(*it)->transaction_ids.empty()) { //transactions_ids can be removed
                //merge.
                pNode->merge_transaction_ids((*it)->transaction_ids);
                //pNode->transaction_ids.merge((*it)->transaction_ids);
                //pNode->support = pNode->transaction_ids.size();
                //
            }
        }

    }
    pNode->support = pNode->transaction_ids.size();
    //diff_set can be used instead of transactions_ids
    //pNode->calculate_diff_set_from_children(); //May also calculate support

//    if(!pNode->children.empty()){
//         pNode->print();
//    }
}

void taxonomy_tree::create_vertical_representation()
{
    if(tree_vertical_representation) {
        delete tree_vertical_representation;
    }
    tree_vertical_representation = new std::unordered_map<unsigned int, std::set<unsigned int> *>;
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        create_vertical_representation((*it).second);
    }
}

void taxonomy_tree::create_vertical_representation(taxonomy_node *pNode)
{
    if(!pNode->children.empty()) {
//        pNode->print();
        tree_vertical_representation->insert_or_assign(pNode->element,
                                                       &pNode->transaction_ids);//pointer to transaction_ids
        for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
            this->create_vertical_representation((*it));
        }
    }
}

taxonomy_node *taxonomy_tree::find(taxonomy_node *pNode, unsigned int element, bool &found)
{
    taxonomy_node *found_node = nullptr;
    if(!found) {
        if(pNode->element == element) {
            found = true;
            found_node = pNode;
            return found_node;
        }
        for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
            this->find((*it), element, found);
        }
    }
    return found_node;
}

taxonomy_node *taxonomy_tree::find(unsigned int element)
{
    //Search for element
    bool found = false;
    taxonomy_node *found_node = nullptr;
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        found_node = this->find((*it).second, element, found);
        if(found) {
            break;
        }
    }
    return found_node;
}

void taxonomy_node::print()
{
    cout << this << " element: " << this->element << " parent: "
         << (this->parent == nullptr ? 0 : this->parent->element) << " [" << this->parent << "]" << " sup: "
         << this->support << endl << " Tids: ";
    for(auto it = this->transaction_ids.begin(); it != this->transaction_ids.end(); ++it) {
        cout << (*it) << ", ";
    }
    cout << endl << "diff_set: ";
    for(auto it = this->diff_set.begin(); it != this->diff_set.end(); ++it) {
        cout << (*it) << ", ";
    }
    cout << endl;
}

void taxonomy_node::merge_transaction_ids(const std::set<unsigned int> &with_set)
{
    if(this->transaction_ids.empty()) {
        this->transaction_ids = with_set;
    } else {
        for(auto it = with_set.begin(); it != with_set.end(); ++it) {
            this->transaction_ids.insert((*it));
        }
    }
}

void taxonomy_node::calculate_diff_set_from_children()
{
#if DEBUG_LEVEL > 0
    cout << endl << this->element << " # of children: " << this->children.size() << endl;
#endif
    if(this->children.empty()) {
        return;
    }
    if(!this->diff_set.empty()) {
        this->diff_set = std::set<unsigned int>(); //make it empty
        cerr << "this->diff_set already calculated" << endl;
    }
    //If node has only one child than just copy diff_set
    if(this->children.size() == 1) {
        auto it = this->children.begin();
        this->diff_set = (*it)->diff_set;
        this->support = (*it)->support;
#if DEBUG_LEVEL > 0
        cout << "==1 sup from tids: " << this->support << endl;
#endif
    } else {
        //pNode diff_set is common part of all children
        //Find intersection with all children
        auto size = children.size();
        if(size == 0) return;

        int no_of_finished = 0;
        std::set<unsigned int>::iterator iter[size];
//        bool finished[size];
//        for(int i=0;i<size;++i){
//            finished[i] = false;
//        }
//        unsigned int max_diff_set_size = 0;
        int i = 0;
        for(auto it = children.begin(); it != children.end(); ++it) {
//            if(max_diff_set_size < (*it)->diff_set.size()){
//                max_diff_set_size = (*it)->diff_set.size();
//            }
            iter[i] = (*it)->diff_set.begin();
            ++i;
        }

        auto it0 = iter[0];
        auto c_s = children[0]->diff_set.size();
        int equal[c_s];
        for(i = 0; i < c_s; ++i) {
            equal[i] = 1;
        }

        int el_idx = 0;
        while(it0 != children[0]->diff_set.end()) {
            for(int i = 1; i < size; ++i) {
                if(iter[i] == children[i]->diff_set.end()) {
                    //finished[i] = true;
                    ++no_of_finished;
                    continue;
                }
#if DEBUG_LEVEL > 0
                cout << i << ") it0: " << *it0 << " iter[i]: " << *iter[i] << endl;
#endif
                if(*it0 == *iter[i]) {
                    equal[el_idx]++;
                    ++iter[i];
                } else if(*it0 < *iter[i]) {
                    ++it0; //Go to next element
                    ++el_idx;
                } else {
                    ++iter[i];
                }
                if(equal[el_idx] == size) {
                    //Add to common set
                    this->diff_set.emplace(*it0);
                    ++it0;
                    ++el_idx;
                    ++iter[i];
                    break;
                }
            }
            if(no_of_finished == size - 1) {
                break;
            }
        }
#if DEBUG_LEVEL > 0
        cout << "sup from tids: " << this->support << endl;
#endif
//        This formula is NOT correct.
//        this->support = max_diff_set_size - this->diff_set.size();
    }
}





void taxonomy_tree::print_frequent_itemset(const std::string &file)
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
            myfile << "length\tsup\tdiscovered_frequent_itemset" << endl;
        }
    }
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        this->print_frequent_itemset((*it).second, ascendant, myfile, 0);
    }
    if(myfile.is_open()) {
        myfile.close();
    }
}

void
taxonomy_tree::print_frequent_itemset(taxonomy_node *pNode, std::string all_ascendats, std::ofstream &file, int level)
{
    ////format: length, sup, discovered_frequent_itemset
    ++level;
    string parent = all_ascendats;
    if(pNode->children.empty()) {
        return;
    }
    //For STATS
    ++number_of_created_candidates;

    auto search = number_of_created_candidates_and_frequent_itemsets_of_length.find(level);
    std::pair<std::map<unsigned int, pair<unsigned int, unsigned int>>::iterator, bool> inserted;
    if(search == number_of_created_candidates_and_frequent_itemsets_of_length.end()) {
        inserted = number_of_created_candidates_and_frequent_itemsets_of_length.emplace(level,
                                                                                        pair(1, 0));
        search = inserted.first;
    } else {
        (*search).second.first++; //Increase number of candidates
    }
    if(pNode->support > min_sup) {
        ++number_of_frequent_itemsets;
        (*search).second.second++; //Increase number of frequent_itemsets
        //For STATS end

        if(level > 1) {
            parent += "->";
        }

        parent += to_string(pNode->element);

        if(file.is_open()) {
            file << "1" << "\t" << pNode->support << "\t" << pNode->element /*parent*/ << "" << endl;
        } else {
            cout << "1" << "\t" << pNode->support << "\t" << pNode->element /*parent*/ << "" << endl;
        }
    }
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        this->print_frequent_itemset((*it), parent, file, level);
    }
}

void
taxonomy_tree::calculate_support(std::unordered_map<unsigned int, std::set<unsigned int> *> *vertical_representation)
{
    this->set_sup_from_vertical_representation(vertical_representation);
#if DEBUG_LEVEL > 0
    cout<<"------------- after set_sup_from_vertical_representation ------"<<endl;
    this->print();
#endif
    this->set_sup_from_children();
#if DEBUG_LEVEL > 0
    cout<<"------------- after set_sup_from_children --------"<<endl;
    this->print();
#endif
}

void taxonomy_tree::clear_sets_in_nodes()
{
    for(auto it = roots.begin(); it != roots.end(); ++it) {
        clear_sets_in_nodes((*it).second);
    }
}

void taxonomy_tree::clear_sets_in_nodes(taxonomy_node *pNode)
{
    pNode->clear_sets();
    for(auto it = pNode->children.begin(); it != pNode->children.end(); ++it) {
        if((*it) != nullptr) {
            this->clear_sets_in_nodes((*it));
        }
    }
}

