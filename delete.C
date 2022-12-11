#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "catalog.h"
#include "error.h"
#include "heapfile.h"
#include "query.h"


/*
 * Deletes records from a specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

const Status QU_Delete(const string &relation, const string &attrName,
                       const Operator op, const Datatype type,
                       const char *attrValue) {
  // part 6
  Status status;
  RID rid;

  // remove all tuples as no attribute was given
  if (attrName.empty()) {
    HeapFileScan scan(relation, status);
    if (status != OK) {
      return status;
    }
    // start scan with no filter
    status = scan.startScan(0, relation.length() + 1, STRING, NULL, EQ);
    if (status != OK) {
      return status;
    }

    // loop through relation and delete all tuples
    while ((status = scan.scanNext(rid)) != FILEEOF) {
      status = scan.deleteRecord();
      if (status != OK) {
        return status;
      }
    }
    scan.endScan()
    return OK;
  }

  // offset calculation
  int offset = 0;
  AttrDesc attrDesc;
  if (!attrName.empty()) {
    status = attrCat->getInfo(relation, attrName, attrDesc);
    if (status != OK) {
      return status;
    }
    offset = attrDesc.attrOffset;
  }

  // filtered HeapFileScan to locate qualifying tuples
  HeapFileScan scan(relation, status);
  if (status != OK) {
    return status;
  }

  int intFilter;
  float floatFilter;

  // start scan (similar to select.C)
  if (type == STRING) {
    status = scan.startScan(offset, strlen(attrValue), type, attrValue, op);
    if (status != OK) {
      return status;
    }
  } else if (type == INTEGER) {
    intFilter = atoi(attrValue);
    status = scan.startScan(offset, sizeof(int), type,
                            reinterpret_cast<char *>(&intFilter), op);
    if (status != OK) {
      return status;
    }
  } else if (type == FLOAT) {
    floatFilter = atof(attrValue);
    status = scan.startScan(offset, sizeof(float), type,
                            reinterpret_cast<char *>(&floatFilter), op);
    if (status != OK) {
      return status;
    }
  }

  while ((status = scan.scanNext(rid)) != FILEEOF) {
    // delete tuple
    status = scan.deleteRecord();
    if (status != OK) {
      return status;
    }
  }

  // end scan
  scan.endScan();
  return OK;
}
