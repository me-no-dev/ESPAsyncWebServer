/*
 * StringArray.h
 *
 *  Created on: 18.12.2015 г.
 *      Author: ficeto
 */

#ifndef STRINGARRAY_H_
#define STRINGARRAY_H_

#include "stddef.h"
#include "WString.h"

class StringArrayItem;
class StringArrayItem {
  private:
    String _string;
  public:
    StringArrayItem *next;
    StringArrayItem(String str):_string(str), next(NULL){}
    ~StringArrayItem(){}
    String string(){ return _string; }
};

class StringArray {
  private:
    StringArrayItem *_items;
  public:
    StringArray():_items(NULL){}
    StringArray(StringArrayItem *items):_items(items){}
    ~StringArray(){}
    StringArrayItem *items(){
      return _items;
    }
    void add(String str){
      StringArrayItem *it = new StringArrayItem(str);
      if(_items == NULL){
        _items = it;
      } else {
        StringArrayItem *i = _items;
        while(i->next != NULL) i = i->next;
        i->next = it;
      }
    }
    size_t length(){
      size_t i = 0;
      StringArrayItem *it = _items;
      while(it != NULL){
        i++;
        it = it->next;
      }
      return i;
    }
    bool contains(String str){
      StringArrayItem *it = _items;
      while(it != NULL){
        if(it->string() == str)
          return true;
        it = it->next;
      }
      return false;
    }
    String get(size_t index){
      size_t i = 0;
      StringArrayItem *it = _items;
      while(it != NULL){
        if(i++ == index)
          return it->string();
        it = it->next;
      }
      return String();
    }
    bool remove(size_t index){
      StringArrayItem *it = _items;
      if(!index){
        if(_items == NULL)
          return false;
        _items = _items->next;
        delete it;
        return true;
      }
      size_t i = 0;
      StringArrayItem *pit = _items;
      while(it != NULL){
        if(i++ == index){
          pit->next = it->next;
          delete it;
          return true;
        }
        pit = it;
        it = it->next;
      }
      return false;
    }
    bool remove(String str){
      StringArrayItem *it = _items;
      StringArrayItem *pit = _items;
      while(it != NULL){
        if(it->string() == str){
          if(it == _items){
            _items = _items->next;
          } else {
            pit->next = it->next;
          }
          delete it;
          return true;
        }
        pit = it;
        it = it->next;
      }
      return false;
    }
    void free(){
      StringArrayItem *it;
      while(_items != NULL){
        it = _items;
        _items = _items->next;
        delete it;
      }
    }
};



#endif /* STRINGARRAY_H_ */
