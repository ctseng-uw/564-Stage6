#include <cstdlib>
#include <cstring>
#include "catalog.h"
#include "error.h"
#include "heapfile.h"
#include "query.h"
#include "stdio.h"
#include "stdlib.h"

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

  // offset calculation
  int offset = 0;
  AttrDesc attrDesc;
  if (attrName != NULL) {
    status = attrCat->getInfo(relation, attrName, attrDesc);
    if (status != OK) {
      return status;
    }
    offset = attrDesc.attrOffset;
  }

  // filtered HeapFileScan to locate qualifying tuples
  HeapFileScan scan(attrName, status);
  if (status != OK) {
    return status;
  }

  int intFilter;
  float floatFilter;

  // start scan (similar to select.C)
  if (type == NULL) {
    status = scan.startScan(offset, 0, type, NULL, EQ);
  } else if (type == STRING) {
    status = scan.startScan(offset, strlen(attrValue), type, attrValue, op);
  } else if (type == INTEGER) {
    intFilter = atoi(attrValue);
    status = scan.startScan(offset, sizeof(int), type,
                            reinterpret_cast<char *>(&intFilter), op);
  } else if (type == FLOAT) {
    floatFilter = atof(attrValue);
    status = scan.startScan(offset, sizeof(float), type,
                            reinterpret_cast<char *>(&floatFilter), op);
  } else {
    // not available data type
    return NOTUSED1;
  }

  RID rid;
  Record record;

  while ((status = scan.scanNext(rid)) != FILEEOF) {
    // Get the record
    status = scan.getRecord(record);
    if (status != OK) {
      return status;
    }
    // delete tuple
    status = attrCat->removeInfo(relation, attrName);
    if (status != OK) {
      return status;
    }
  }

  scan.endScan();
  return OK;
}
