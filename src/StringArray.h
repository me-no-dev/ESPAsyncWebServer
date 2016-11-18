/*
 Asynchronous WebServer library for Espressif MCUs
 
 Copyright (c) 2016 Hristo Gochkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef STRINGARRAY_H_
#define STRINGARRAY_H_

#include "stddef.h"
#include "WString.h"
#include <iterator>

template <typename T>
class ListArrayNode {
  T _item;
public:
  ListArrayNode(const T t): _item(t), next(nullptr) {}
  ~ListArrayNode(){}
  
  const T& item() const { return _item; };
  T& item(){ return _item; }
  
  ListArrayNode<T>* next;
};

template <typename T, template<typename> class Item = ListArrayNode>
class ListArray {
public:
  typedef Item<T> ItemType;
  typedef std::function<void(const T&)> OnRemove;
  typedef std::function<bool(const T&)> Predicate;
  
private:
  ItemType* _root;
  OnRemove _onRemove;

  class Iterator : std::iterator<std::input_iterator_tag, const T> {
    ItemType* _node;
  public:
    Iterator(ItemType* current = nullptr) : _node(current) {}
    Iterator(const Iterator& i) : _node(i._node) {}
    Iterator& operator ++() { _node = _node->next; return *this; }
    bool operator != (const Iterator& i) const { return _node != i._node; }
    const T& operator * () const { return _node->item(); }
    const T* operator -> () const { return &_node->item(); }
  };
  
public:
  typedef const Iterator ConstIterator;
  ConstIterator begin() const { return ConstIterator(_root); }
  ConstIterator end() const { return ConstIterator(nullptr); }
  
  ListArray(OnRemove onRemove) : _root(nullptr), _onRemove(onRemove) {}
  ~ListArray(){}
  
  inline
  T& front() const { return _root->item(); }
  
  inline
  bool isEmpty() const {
    return _root == nullptr;
  }
  
  size_t count() const {
    size_t i = 0;
    auto it = _root;
    while(it){
      i++;
      it = it->next;
    }
    return i;
  }
  
  size_t count_if(Predicate predicate) const {
    size_t i = 0;
    auto it = _root;
    while(it){
      if (!predicate){
        i++;
      }
      else if (predicate(it->item())) {
        i++;
      }
      it = it->next;
    }
    return i;
  }
  
  const T& nth(size_t N) const {
    size_t i = 0;
    auto it = _root;
    while(it){
      if(i++ == N)
        return it->item();
      it = it->next;
    }
    return T();
  }
  
  bool remove(const T& t){
    auto it = _root;
    auto pit = _root;
    while(it){
      if(it->item() == t){
        if(it == _root){
          _root = _root->next;
        } else {
          pit->next = it->next;
        }
        
        if (_onRemove) {
          _onRemove(it->item());
        }
        
        delete it;
        return true;
      }
      pit = it;
      it = it->next;
    }
    return false;
  }
  
  bool remove_first(Predicate predicate){
    auto it = _root;
    auto pit = _root;
    while(it){
      if(predicate(it->item())){
        if(it == _root){
          _root = _root->next;
        } else {
          pit->next = it->next;
        }
        if (_onRemove) {
          _onRemove(it->item());
        }
        delete it;
        return true;
      }
      pit = it;
      it = it->next;
    }
    return false;
  }
  
  const T& add(const T& t){
    auto it = new ItemType(t);
    if(_root == nullptr){
      _root = it;
    } else {
      auto i = _root;
      while(i->next != nullptr) i = i->next;
      i->next = it;
    }
    return t;
  }
  
  void free(){
    while(_root != nullptr){
      auto it = _root;
      _root = _root->next;
      if (_onRemove) {
        _onRemove(it->item());
      }
      delete it;
    }
    _root = nullptr;
  }
};


class StringArray : public ListArray<String> {
public:
  
  StringArray() : ListArray(nullptr) {}
  
  bool containsIgnoreCase(const String& str){
    for (const auto& s : *this) {
      if (str.equalsIgnoreCase(s)) {
        return true;
      }
    }
    return false;
  }
};




#endif /* STRINGARRAY_H_ */
