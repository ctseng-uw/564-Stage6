#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

#include "catalog.h"
#include "error.h"
#include "heapfile.h"
#include "query.h"
#include "stdio.h"
#include "stdlib.h"

/*
 * Selects records from the specified relation.
 *
 * Returns:
 * 	OK on success
 * 	an error code otherwise
 */

#define CHK(status)                                                            \
  if ((status) != OK) {                                                        \
    cerr << "Unexpected error at line " << __LINE__ << " (" << __FILE__ << ")" \
         << endl;                                                              \
    Error err;                                                                 \
    err.print((status));                                                       \
    return (status);                                                           \
  }

const Status QU_Select(const string &result, const int projCnt,
                       const attrInfo projNames[], const attrInfo *attr,
                       const Operator op, const char *attrValue) {
  Status status;

  // Get AttrDesc for all the projection and caculate the total record length
  int reclen = 0;
  vector<AttrDesc> projDescs(projCnt);
  for (int i = 0; i < projCnt; i++) {
    status = attrCat->getInfo(projNames[i].relName, projNames[i].attrName,
                              projDescs[i]);
    CHK(status);
    reclen += projDescs[i].attrLen;
  }

  // Figure out the offset of the filtered attribute
  int offset = 0;
  const string &relName = projNames[0].relName;
  if (attr != nullptr) {
    AttrDesc attrDesc;
    status = attrCat->getInfo(relName, attr->attrName, attrDesc);
    CHK(status);
    offset = attrDesc.attrOffset;
  }

  // Figure out the filter type, value and start the scan
  HeapFileScan scan(relName, status);
  CHK(status);
  assert(status == OK);
  // The following 2 variables need to have longer life time than `scan`,
  // since HeapFileScan read them during scan
  int filteri;
  float filterf;
  if (attr == nullptr) {
    status = scan.startScan(offset, 0, STRING, nullptr, EQ);
  } else if (attr->attrType == STRING) {
    status = scan.startScan(offset, strlen(attrValue), STRING, attrValue, op);
  } else if (attr->attrType == INTEGER) {
    filteri = atoi(attrValue);
    status = scan.startScan(offset, sizeof(int), INTEGER,
                            reinterpret_cast<char *>(&filteri), op);
  } else if (attr->attrType == FLOAT) {
    filterf = atof(attrValue);
    status = scan.startScan(offset, sizeof(float), FLOAT,
                            reinterpret_cast<char *>(&filterf), op);
  } else {
    // Unknow type, return an error
    CHK(NOTUSED1);
  }
  CHK(status);

  // Start the insert scan
  InsertFileScan iscan(result, status);
  CHK(status);
  RID rid;

  // Create a dummy record
  Record recordRead;
  Record recordWrite;
  auto recordWriteBuf = make_unique<char[]>(reclen);
  recordWrite.length = reclen;
  recordWrite.data = recordWriteBuf.get();

  // Loop until no record
  while ((status = scan.scanNext(rid)) != FILEEOF) {
    CHK(status);
    // Get the record
    status = scan.getRecord(recordRead);
    CHK(status);
    // Write each column into the correct offset
    char *cur = recordWriteBuf.get();
    for (const AttrDesc &desc : projDescs) {
      memcpy(cur, static_cast<char *>(recordRead.data) + desc.attrOffset,
             desc.attrLen);
      cur += desc.attrLen;
    }
    // Insert to the result table
    // iscan will copy recordWrite, so we can overwrite it in the next iteration
    status = iscan.insertRecord(recordWrite, rid);
    CHK(status);
  }

  scan.endScan();
  return OK;
}
