#ifndef TEMPL_LINKED_LIST_H
#define TEMPL_LINKED_LIST_H

#include "base.h"

template <typename T>
class LinkList {
public:
  LinkList();

  LinkList(const LinkList<T> &sourcell);
  ~LinkList();

  void operator=(const LinkList<T> &sorucell);

  Boolean operator==(const LinkList<T> &rhs) const;

  operator T *();

  void Append(const T &item);
  void Append(const LinkList<T> &sourcell);

  LinkList<T> operator+(const LinkList<T> &sourcell) const;
  LinkList<T> operator+(const T &element) const;
  void operator+=(const LinkList<T> &sourcell);
  void operator+=(const T &element);

  void Delete(int index);

  void Erase();

  T *Get(int index);

  T* operator[](int index);

  int GetLength() const;

protected:
    // An internal node within the linked list is a single element.  It
    // contains the data that we are storing (of type T defined by the
    // template) and pointers to the previous and next.
    struct InternalNode {
          T Data;
          InternalNode * next;
          InternalNode * previous;
    };

    int iLength;               // # of items in list
    InternalNode *pnHead;      // First item
    InternalNode *pnTail;      // Last item
    InternalNode *pnLastRef;   // Current item
    int iLastRef;              // Which element # last referenced

    // Set the Private members of the class to initial values
    void SetNull();
};

// SetNull
//
// This will set (or reset) the intial values of the private members of
// LinkList
//
template <class T>
inline void LinkList<T>::SetNull() {
  pnHead = nullptr;
  pnTail = nullptr;
  pnLastRef = nullptr;
  iLength = 0;
  iLastRef = -1;
}

/****************************************************************************/

//
// LinkList
//
// Constructor
//
template <class T>
inline LinkList<T>::LinkList () {
    SetNull();
}

/****************************************************************************/

//
// LinkList ( const LinkList<T> &sourcell )
//
// Copy constructor
//
template <class T>
LinkList<T>::LinkList ( const LinkList<T> & sourcell ) {
  // Initialize the new list
  SetNull();

  // And copy all the members of the passed in list
  if (sourcell.iLength == 0)
    return;

  InternalNode *n = sourcell.pnHead;

  while (n != nullptr) {
    Append(n->Data);
    n = n->next;
  }
  pnLastRef = pnHead;
}

/****************************************************************************/

//
// ~LinkList
//
// Destructor
//
template <class T>
inline LinkList<T>::~LinkList() {
  Erase();
}

/****************************************************************************/

//
// Operator =
//
// Assignment operator
//
template <class T>
void LinkList<T>::operator = ( const LinkList<T> & sourcell ) {
  // First erase the original list
  Erase();

  // Now, copy the passed in list
  InternalNode *pnTemp = sourcell.pnHead;

  while (pnTemp != nullptr) {
    Append(pnTemp->Data);
    pnTemp = pnTemp->next;
  }

  pnLastRef = nullptr;
  iLastRef = -1;
}

/****************************************************************************/

//
// Operator ==
//
// Test for equality of two link lists
//
template <class T>
Boolean LinkList<T>::operator == ( const LinkList<T> & rhs ) const {
  if (iLength != rhs.iLength)
    return (FALSE);

  InternalNode *pnLhs = this->pnHead;
  InternalNode *pnRhs = rhs.pnHead;

  while (pnLhs != nullptr && pnRhs != nullptr) {
    // The Data type T set by the template had better define an equality
    // operator for their data type!
    if (!(pnLhs->Data == pnRhs->Data))
      return FALSE;
    pnLhs = pnLhs->next;
    pnRhs = pnRhs->next;
  }

  if (pnLhs==nullptr && pnRhs==nullptr)
    return TRUE;
  else
    return FALSE;
}
/****************************************************************************/

//
// Conversion to array operator
//
// This returns a copy of the list and the caller must delete it when done.
//
template <class T>
LinkList<T>::operator T * () {
  if (iLength == 0)
    return nullptr;

  T *pResult = new T[iLength];

  InternalNode *pnCur = pnHead;
  T *pnCopy = pResult;

  while (pnCur != nullptr) {
    *pnCopy = pnCur->Data;
    ++pnCopy;
    pnCur = pnCur->next;
  }

  // Note:  This is a copy of the list and the caller must delete it when
  // done.
  return pResult;
}

/****************************************************************************/

//
// Append
//
// Append new item to the end of the linked list
//
template <class T>
inline void LinkList<T>::Append ( const T & item ) {
  InternalNode *pnNew = new InternalNode;

  pnNew->Data = item;
  pnNew->next = nullptr;
  pnNew->previous = pnTail;

  // If it is the first then set the head to this element
  if (iLength == 0) {
    pnHead = pnNew;
    pnTail = pnNew;
    pnLastRef = pnNew;
  } else {
    // Set the tail to be this new element
    pnTail->next = pnNew;
    pnTail = pnNew;
  }

  ++iLength;
}

/****************************************************************************/

template <class T>
inline LinkList<T> LinkList<T>::operator+(const LinkList<T> &sourcell) const {
  LinkList<T> pTempLL(*this);
  pTempLL += sourcell;
  return pTempLL;
}

/****************************************************************************/

template <class T>
inline LinkList<T> LinkList<T>::operator+(const T &element) const {
  LinkList<T> pTempLL(*this);
  pTempLL += element;
  return pTempLL;
}

/****************************************************************************/

template <class T>
void LinkList<T>::operator+=(const LinkList<T> &list) {
  const InternalNode *pnTemp;
  const int iLength = list.iLength;
  int i;

  // Must use size as stopping condition in case *this == list.
  for (pnTemp = list.pnHead, i=0; i < iLength; pnTemp = pnTemp->next, i++)
    *this += pnTemp->Data;
}

/****************************************************************************/

template <class T>
void LinkList<T>::operator+=(const T &element) {
  InternalNode *pnNew = new InternalNode;
  pnNew->next = nullptr;
  pnNew->Data = element;
  if (iLength++ == 0) {
    pnHead = pnNew;
    pnNew->previous = nullptr;
  } else {
    pnTail->next = pnNew;
    pnNew->previous = pnTail;
  }
  pnTail = pnNew;
}

/****************************************************************************/

template <class T>
void LinkList<T>::Append ( const LinkList<T> & sourcell ) {
  const InternalNode *pnCur = sourcell.pnHead;

  while (pnCur != nullptr) {
    Append(pnCur->Data);
    pnCur = pnCur->next;
  }
}

/****************************************************************************/

//
// Delete
//
// Delete the specified element
//
template <class T>
inline void LinkList<T>::Delete(int which) {
  if (which>iLength || which == 0)
    return;

  InternalNode *pnDeleteMe = pnHead;

  for (int i=1; i<which; i++)
    pnDeleteMe = pnDeleteMe->next;

  if (pnDeleteMe == pnHead) {
    if (pnDeleteMe->next == nullptr) {
      delete pnDeleteMe;
      SetNull();
    } else {
      pnHead = pnDeleteMe->next;
      pnHead->previous = nullptr;
      delete pnDeleteMe;
      pnLastRef = pnHead;
    }
  } else {
    if (pnDeleteMe == pnTail) {
      if (pnDeleteMe->previous == nullptr) {
        delete pnDeleteMe;
        SetNull();
      } else {
        pnTail = pnDeleteMe->previous;
        pnTail->next = nullptr;
        delete pnDeleteMe;
        pnLastRef = pnTail;
      }
    } else {
      pnLastRef = pnDeleteMe->next;
      pnDeleteMe->previous->next = pnDeleteMe->next;
      pnDeleteMe->next->previous = pnDeleteMe->previous;
      delete pnDeleteMe;
    }
  }

  if (iLength!=0)
    --iLength;
}

/****************************************************************************/

template <class T>
inline T* LinkList<T>::operator[](int index) {
  return (Get(index));
}

/****************************************************************************/

//
// Erase
//
// remove all items from the list
//
template <class T>
inline void LinkList<T>::Erase() {
  pnLastRef = pnHead;

  while (pnLastRef != nullptr) {
    pnHead = pnLastRef->next;
    delete pnLastRef;
    pnLastRef = pnHead;
  }
  SetNull();
}

/****************************************************************************/

// Get
//
// Get a specified item.  Notice here that index can be between 0 and
// 1-iLength.  Once we determine this I add 1 to the index in order to make
// the get function easier.
//
template <class T>
inline T* LinkList<T>::Get(int index) {
  int iCur;               // Position to start search from
  InternalNode *pnTemp;   // Node to start search from
  int iRelToMiddle;       // Position asked for relative to last ref

  // Make sure that item is within bounds
  if (index < 0 || index >= iLength)
    return nullptr;

  // Having the index be base 1 makes this procedure much easier.
  index++;

  if (iLastRef==-1) {
    if (index < (iLength-index)) {
      iCur = 1;
      pnTemp = pnHead;
    } else {
      iCur = iLength;
      pnTemp = pnTail;
    }
  } else {
    if (index < iLastRef) {
      iRelToMiddle = iLastRef - index;
    } else {
      iRelToMiddle = index - iLastRef;

      if (index < iRelToMiddle) {
        // The head is closest to requested element
        iCur = 1;
        pnTemp = pnHead;
      } else {
        if (iRelToMiddle < (iLength - index)) {
          iCur = iLastRef;
          pnTemp = pnLastRef;
        } else {
          iCur = iLength;
          pnTemp = pnTail;
        }
      }
    }
  }

  // Now starting from the decided upon first element
  // find the desired element
  while (iCur != index) {
    if (iCur < index) {
      iCur++;
      pnTemp = pnTemp->next;
    } else {
      iCur--;
      pnTemp = pnTemp->previous;
    }
  }

  iLastRef = index;
  pnLastRef = pnTemp;

  return &(pnLastRef->Data);
}

/****************************************************************************/

template <class T>
inline int LinkList<T>::GetLength() const {
  return iLength;
}

#endif