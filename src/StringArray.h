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
