/****************************************************************************/
/*                  Copyright 2001, Trustees of Boston University.          */
/*                               All Rights Reserved.                       */
/*                                                                          */
/* Permission to use, copy, or modify this software and its documentation   */
/* for educational and research purposes only and without fee is hereby     */
/* granted, provided that this copyright notice appear on all copies and    */
/* supporting documentation.  For any other uses of this software, in       */
/* original or modified form, including but not limited to distribution in  */
/* whole or in part, specific prior permission must be obtained from Boston */
/* University.  These programs shall not be used, rewritten, or adapted as  */
/* the basis of a commercial software or hardware product without first     */
/* obtaining appropriate licenses from Boston University.  Boston University*/
/* and the author(s) make no representations about the suitability of this  */
/* software for any purpose.  It is provided "as is" without express or     */
/* implied warranty.                                                        */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*  Author:    Alberto Medina                                               */
/*             Anukool Lakhina                                              */
/*  Title:     BRITE: Boston university Representative Topology gEnerator   */
/*  Revision:  2.0         4/02/2001                                        */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*  Modified lightly to easily interface with ns-3                          */
/*  Author:     Josh Pelkey <jpelkey@gatech.edu>                            */
/*  Date: 3/02/2011                                                         */
/****************************************************************************/
#ifndef MODEL_H
#define MODEL_H
#pragma interface

#include "../Graph.h"
#include "../Parser.h"
#include <algorithm>

namespace brite {

enum PlacementType { P_RANDOM = 1, P_HT = 2 };
enum GrowthType { G_INCR = 1, G_ALL = 2 };
enum PrefType { PC_NONE = 1, PC_BARABASI = 2 };
enum ConnLocType { CL_ON = 1, CL_OFF = 2 };
enum BWDistType { BW_CONST = 1, BW_UNIF = 2, BW_EXP = 3, BW_HT = 4};
enum DelayDistType { D_DISTANCE = 1, D_TECH = 2};
enum ModelType {RT_WAXMAN = 1, RT_BARABASI = 2, 
		 AS_WAXMAN = 3, AS_BARABASI = 4,
		 TD_HIER = 5, BU_HIER = 6, IF_ROUTER = 7, IF_AS = 8};
enum EdgeConnType { TD_RANDOM = 1, TD_SMALLEST = 2, 
		    TD_SMALLEST_NOLEAF = 3, TD_K_DEGREE = 4};
enum GroupingType { BU_RANDOM_PICK = 1, BU_RANDOM_WALK = 2 };
enum AssignmentType { A_CONST = 1, A_UNIF = 2, A_EXP = 3, A_HT = 4 };

class Graph;
class BriteNode;

//////////////////////////////////////////////
//
// class Model
// Base class for all generation models
//
//////////////////////////////////////////////

class PlaneRowAdjNode {

 public:
  PlaneRowAdjNode(int tx) { x = tx; }
  int GetX() { return x; }
  bool ColFind(int ty);
  void ColInsert(int ty);
  
 private:
  int x;
  std::list<int> row_adjlist;

};

class Model {
  
  friend class RandomVariable;

 public:

  Model() {};
  virtual ~Model() {};

  virtual Graph* Generate() {return (Graph*)NULL;}
  void PlaceNodes(Graph* g);
  int GetPlacementType() { return NodePlacement; }
  int GetGrowthType() { return Growth; }
  int GetPrefType() { return PrefConn; }
  int GetConnLocType() { return ConnLoc; }
  int GetSize() { return size; }  
  int GetScale() { return Scale_1; }
  int GetX() { return X; }
  int GetY() { return Y; }
  int GetX_max() { return X_max; }
  int GetY_max() { return Y_max; } 
  void SetX(int x) {  X=x; }
  void SetY(int y) { Y=y; }
  void SetX_max(int x) {  X_max=x; }
  void SetY_max(int y) { Y_max=y; }
  ModelType GetType() { return type; }
  int GetMEdges() { return m_edges; }
  std::string ToString();
  bool PlaneCollision(int tx, int ty);
  
  /* Random Variable seeds */
  static unsigned short int s_places[3];
  static unsigned short int s_connect[3];
  static unsigned short int s_edgeconn[3];
  static unsigned short int s_grouping[3];
  static unsigned short int s_assignment[3];
  static unsigned short int s_bandwidth[3];

 protected:

  PlacementType NodePlacement;
  GrowthType Growth;
  PrefType PrefConn;
  ConnLocType ConnLoc;
  ModelType type;
  int Scale_1;
  int Scale_2;
  int X;
  int Y;
  int X_max;
  int Y_max;  
  int m_edges;
  int size;
  static std::vector<PlaneRowAdjNode*> row_ocup;

};

} // namespace brite

#endif /* MODEL_H */
