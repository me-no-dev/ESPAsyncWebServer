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
#include <algorithm>

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
  typedef std::function<void(T)> OnItem;
  typedef std::function<bool(T)> Predicate;
  
private:
  OnItem _onDelete;
  ItemType* _root;
public:

  class Iterator{
    ItemType* _current;
  public:
    Iterator(ItemType* current = nullptr) : _current(current) {}
    Iterator(const Iterator& i) : _current(i._current) {}
    Iterator& operator ++(){ _current = _current->next; return *this;  }
    bool operator == (const Iterator& i) const { return _current == i._current; }
    bool operator != (const Iterator& i) const { return _current != i._current; }
    T& operator * (){ return _current->item(); }
    T* operator -> (){ return &_current->item(); }
  };
  Iterator begin() { return Iterator(_root); }
  Iterator end() { return Iterator(nullptr); }
  
  class ConstIterator{
    ItemType* _current;
  public:
    ConstIterator(ItemType* current = nullptr) : _current(current) {}
    ConstIterator(const ConstIterator& i) : _current(i._current) {}
    ConstIterator(const Iterator& i) : _current(i._current) {}
    ConstIterator& operator ++(){ _current = _current->next; return *this;  }
    bool operator == (const ConstIterator& i) const { return _current == i._current; }
    bool operator != (const ConstIterator& i) const { return _current != i._current; }
    const T& operator * () const { return _current->item(); }
    const T* operator -> () const { return &_current->item(); }
  };
  ConstIterator begin() const { return ConstIterator(_root); }
  ConstIterator end() const { return ConstIterator(nullptr); }
  
  
  ListArray(OnItem onDelete) : _root(nullptr), _onDelete(onDelete) {}
  ~ListArray(){
    free();
  }
  
  inline
  T& front() const { return _root->item(); }
  
  inline
  bool isEmpty() const {
    return _root == nullptr;
  }
  
  size_t length() const {
    size_t i = 0;
    ItemType *it = _root;
    while(it != nullptr){
      i++;
      it = it->next;
    }
    return i;
  }
  
  bool contains(T t){
    ItemType *it = _root;
    while(it != nullptr){
      if(it->item() == t)
        return true;
      it = it->next;
    }
    return false;
  }
  
  T get_nth(size_t index) const {
    size_t i = 0;
    ItemType *it = _root;
    while(it != nullptr){
      if(i++ == index)
        return it->item();
      it = it->next;
    }
    return T();
  }
  
  bool remove(T t){
    ItemType *it = _root;
    ItemType *pit = _root;
    while(it != nullptr){
      if(it->item() == t){
        if(it == _root){
          _root = _root->next;
        } else {
          pit->next = it->next;
        }
        if (_onDelete) {
          _onDelete(it->item());
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
    ItemType *it = _root;
    ItemType *pit = _root;
    while(it != nullptr){
      if(predicate(it->item())){
        if(it == _root){
          _root = _root->next;
        } else {
          pit->next = it->next;
        }
        if (_onDelete) {
          _onDelete(it->item());
        }
        delete it;
        return true;
      }
      pit = it;
      it = it->next;
    }
    return false;
  }
  
  T add(T t){
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
      if (_onDelete) {
        _onDelete(it->item());
      }
      delete it;
    }
    _root = nullptr;
  }
};



class StringArray : public ListArray<String> {
public:
  
  StringArray() : ListArray(nullptr) {}
  
  bool containsIgnoreCase(String str){
    for (const auto& s : *this) {
      if (str.equalsIgnoreCase(s)) {
        return true;
      }
    }
    return false;
  }
};




#endif /* STRINGARRAY_H_ */
