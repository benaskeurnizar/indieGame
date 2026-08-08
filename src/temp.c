

for(int i=0;i<num_points+2;i++){
    float x,y;
    if(i>=num_points){
      int index = i-num_points;
      x = super_x[index];
      y = super_y[index];
    }else{
      x = xs[i];
      y = ys[i];
    }
    if(x==-999.0 && y==-999.0){
      return;
    }
    int num_edges = 50;
    Edge* edge_buffer = (Edge*)malloc(sizeof(Edge)*num_edges);
    int edge_index = 0;
    int triangle_write = 0;
    for(int j=0;j<triangle_index;j++){
      triangle2D tri = plane_triangles[j];
      triangleCircumCircle(&tri);
      int delete_triangle = 0;
      if(pointInTriangle(tri,x,y)){
	//TODO: delete triangle from triangles buffer
	delete_triangle = 1;
	//TODO: mind the edge_buffer size
	edge_buffer[edge_index++] = Edge{tri.x1,tri.y1,tri.x2,tri.y2};
	edge_buffer[edge_index++] = Edge{tri.x2,tri.y2,tri.x3,tri.y3};
	edge_buffer[edge_index++] = Edge{tri.x3,tri.y3,tri.x1,tri.y1};
      }
      if(!delete_triangle){
	plane_triangles[triangle_write++] = tri;
      }
    }
    triangle_index = triangle_write;
    //TODO: delete all doubly specified edges from the edge buffer
    int edge_write = 0;
    for(int k=0;k<edge_index;k++){
      Edge edge1 = edge_buffer[k];
      int duplicate = 0;
      for(int l=0;l<edge_index;l++){	
	if(k!=l){
	  Edge edge2 = edge_buffer[l];
	  if(((edge1.x1==edge2.x1 && edge1.y1==edge2.y1) && (edge1.x2==edge2.x2 && edge1.y2==edge2.y2)) || ((edge1.x1==edge2.x2 && edge1.y1==edge2.y2) && (edge1.x2==edge2.x1 && edge1.y2==edge2.y1))){
	    duplicate = 1;
	    break;
	  }
	}
      }
      if(!duplicate){
	edge_buffer[edge_write++] = edge1;
      }
    }
    edge_index = edge_write;

    //TODO: add all the triangles between our point and edges to the triangle list
    for(int j=0;j<edge_index;j++){
      Edge e = edge_buffer[j];
      triangle2D triangle = triangle2D{e.x1,e.x2,x,e.y1,e.y2,y,0.0,0.0,0.0};
      addTriangle(&plane_triangles,&triangle_index,&num_triangles,triangle);
    }
    free(edge_buffer);
  }
