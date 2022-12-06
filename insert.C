#include <memory>
#include "catalog.h"
#include "query.h"
#include "error.h"
#include "heapfile.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"


/*
 * Inserts a record into the specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Insert(const string &relation, const int attrCnt,
                       const attrInfo attrList[]) {
  Status status;

  // first check if the number of attribute is correct
  RelDesc relDesc;
  status = relCat->getInfo(relation, relDesc);
  if (status != OK){
    return status;
  }
  if (relDesc.attrCnt != attrCnt){
    // need to return an error because NULL is not supported in Minirel
    return NOTUSED1;
  }

  // figure out the total length and the order of a record
  int reclen = 0;
  int tempAttrCnt = 0;
  AttrDesc* allAttr;  
  status = attrCat->getRelInfo(relation, tempAttrCnt, allAttr);
  if (status != OK){
    return status;
  }

  //calculate record length
  for (int i = 0; i < tempAttrCnt; i++){
    reclen = reclen + allAttr[i].attrLen;
  }

  // start the insert scan
  InsertFileScan insertScan(relation, status);
  RID rid;
  if (status != OK){
    return status;
  }

  // create dummy record (following from select.C)
  auto recordWriteBuf = make_unique<char[]>(reclen);
  Record recordWrite;
  recordWrite.length = reclen;
  recordWrite.data = recordWriteBuf.get();

  // idea : use double loops to put in the attributes in order
  int containeri;
  float containerf;
  char *cur = recordWriteBuf.get();
  for (int i = 0; i < tempAttrCnt; i++){
    for (int j = 0; j < tempAttrCnt; j++){
      // if the attrbute names are the same, we can put it in to the record
      if (strcmp(allAttr[i].attrName, attrList[j].attrName) == 0){
        // We need to care about the data type when we copy it into the cur pointer.
        char* inputAttribute;

        if (attrList[j].attrType == STRING){
          inputAttribute = (char *)attrList[j].attrValue;
        } else if (attrList[j].attrType == INTEGER){
          containeri = atoi((char *)attrList[j].attrValue);
          inputAttribute = (char *)containteri;
        } else if (attrList[j].attrType == FLOAT){
          containerf = atof((char *)attrList[j].attrValue);
          inputAttribute = (char *)containterf;
        }

        memcpy(cur, 
                inputAttribute,
                allAttr[i].attrLen);
        cur = cur + allAttr[i].attrLen;              
        break;
      }
    }
  }

  status = insertScan.insertRecord(recordWrite, rid);
  if (status != OK){
    return status;
  }

  return OK;
}
