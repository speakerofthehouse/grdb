//code developed by eric speaker
//code implimentation taken from research of frank miiller through code
	//files provided by him
//db project3

//various includes needed for various things within the function
#include "graph.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cli.h"
#include <limits.h>
//#if _DEBUG
#include <assert.h>


/* Place the code for your Dijkstra implementation in this file */


int
component_sssp(component_t c,vertexid_t v1,vertexid_t v2,int *n,
        int *total_weight,vertexid_t **path){

/*
within the intial function creation a bunch of functions created to do various
	things that will be applied in the djikstra algorithm. this includes
	matrix and array creation, updating and search for specific things
	that would be needed for lines of the djikstra algorithm
*/
/*
these functions normally would be created outside of the component_sssp function
however it's easier to write them as part of/within the sssp function to reference
variables used in the declaration of that function
*/


//this function is created to specifically get the edge weight between 2 vertices
//that are specifically defined by the user or the application of djikstra for the 
//sssp function
int get_edge_weight(vertexid_t a, vertexid_t b){
	//initialize needed variables, this includes things needed for buf/memory access
	char s[BUFSIZE];
	int fd1; //fd needed for file access
	int weight = -1;
	int offset; //memory offset
//component struct access from graph.h
	struct edge e;
	attribute_t attr;

	//open edge file
	memset(s,0,BUFSIZE);
	sprintf(s, "%s/%d/%d/e", grdbdir, gno, cno);	
	fd1 = open(s,O_RDONLY);

	//get edge -- this is getting the edge through struct component->edge from
	//componenet struct in graph.h
	//also calling function used/given for their specific files (see functions at bottom of graph.h)
	edge_init(&e);
	edge_set_vertices(&e,a,b);
	edge_read(&e, c->se, fd1);

	//read edge weight -- based on the data and structs accesses
	//also handle offsets appropriatley becuase of use directly to/from memory
	//these read/writes might not be placed directly next to/near eachother
		//or there might be extra bytes,etc at the end of reads/writes/files	
	attr = e.tuple->s->attrlist;
	offset = tuple_get_offset(e.tuple,attr->name);
	if(offset >= 0){
		weight = tuple_get_int(e.tuple->buf + offset);
	}
	close(fd1);
	return(weight);
}

	//*total_weight = get_edge_weight(v1,v2);

/*
function built to get the number of vertices in the used graph to help for various
additional functions and activities -- this includes building arrays, matricies based
on how many verticies needed
*/
int getnvert(){
	//initialize variables needed and stuff needed for handing memory reads
	off_t offset;
	ssize_t len;
	char s[BUFSIZE];
	int length;
	int fd1;
	//int weight;
	length = sizeof(vertex_t);
	int num_vert = 0;
	char *buf = malloc(length);
	
	//struct edge e;
	//attribute_t attr;
	
	//read from memory
	memset(s,0,BUFSIZE);
	sprintf(s, "%s/%d/%d/v", grdbdir, gno, cno );	
	fd1 = open(s,O_RDONLY);
	
	//set specific location for reads based on offset, lseek sets a specific spot in memory
	//handle the data read accordingly
	for(offset = 0;;offset += length){
		lseek(fd1, offset, SEEK_SET);
		len = read(fd1, buf, length);
		if(len <= 0){
			break;
			num_vert = -1;
		}
	num_vert = num_vert+1;
	}
	close(fd1);
	
	//printf("%i",num_vert);
	return num_vert;	
}
//set a variable to be passed into functions and used in djikstra that is the number
//of vertices found in that getnvert function
int num_vert = getnvert();
	

//initialize the cost and adjacency matricies
	//cost matrix is a matrix that should hold the cost to get from each node to the other
		//x-y axis are the verticies and each point is the dist from the x-vert to the y-vert
		//each spot thats the same vert to itself would be 0
	//adjacency matrix holds a 1 or a 0 to tell if the verticies given by the x & y axis have an edge connection
		//1 = connection/edge exists
		//0 = no edge/no connection or it's itself
	//these are built with the get_edge_weight function to simplify what has to be used
		//in the djikstra algorithm and the results are needed based of that g_e_v function
void matx_init(int num_vert, int cost[num_vert][num_vert], int adj[num_vert][num_vert]){
	
	for(int i=0; i<num_vert; i++){
		for(int j=0; j<num_vert; j++){
			if(get_edge_weight(i,j) >= 0){
				cost[i][j] = get_edge_weight(i,j);
				adj[i][j] = 1; //1 mean edge/connection exists, 0 means no edge
			}
			else{
				cost[i][j] = -1; //-1 means no cost/DNE
				adj[i][j] = 0; //the conection does not exist
			}		
		}
	}		
}

//creates a matrix to hold a 'flag' to tell the user if the vertex has been visited
//or not. this is needed for some parts of djikstra
//sets everything to 0 -- vertex has not been visisted
	//what = visited -- to be determined by other functions
void vertex_visit_init(int num_vert, int flag[num_vert]){
	for(int i=0; i<num_vert; i++){
		flag[i] = 0;
	}
}
//function to search the vertex_visisted array for the specific vertex (given by its integer)
	//returns the result of that spot in array
int vertex_visited(vertexid_t a, int num_vert, int flag[num_vert]){
	return flag[a];
}
//function to update that vertex point once it has been visited, if it's 0 then
//update it to 1 to represent that it has been visited and return that update
//have to handle case where vertex was already 1...this returns out 0 which should 'error'
int vertex_update(vertexid_t a, int num_vert, int flag[num_vert]){
	if(flag[a] == 0){
		flag[a]=1;
		return 1;
	}
	else{
		return 0;
	}
}
//function to check if every vertex has been explored
//would then return a 1 or 0 based on if that happen and if it's 0 = not explored
//it returns out at that point for djikstra to run from that point
int vertex_all_explored(int num_vert, int flag[num_vert]){
	for(int i=0;i<num_vert; i++){
		if(flag[i] == 0){
			return 0;
		}
	}
	return 1;
}
//this is the least weight calculation for djikstra
//pass all needed matricies and then compare all adjacent points from one vertex
//and store only the least value and return the vertex value of that shortest weight 
int least_weight_calc(int num_vert, int flag[num_vert], int cost[num_vert][num_vert], int adj[num_vert][num_vert]){
	int shortestvertex = 0;	
	//int edge_weight;
	int least_edge_weight;
	int j = 0;

	for(int i=0; i<num_vert; i++){
		if(flag[i] == 1){
			if( (adj[i][j] == 1) && (flag[j] == 0) ){
				if(cost[i][j] < least_edge_weight){
					least_edge_weight = cost[i][j];
					shortestvertex = i;			
				}				
			}
		}
	}	
	return shortestvertex;	
}

/*
min wight = infinity, parent, new
	for i in explored
		for i in i.adj
			if i not in expl
				if weight(i,j) < min_weight
					min_weight = weight(i,j)
		parent = i;
		new = j;
*/
//create a matri of distance
//this matrix holds all the values from your start/currently using vertex
//to every other vertex value. if there is no connection that spot would hold
//whatever the intmax is...aka infinity
//ex: say currently at vertex #2
	//[vertex] = [1, 2 ,3,4,5,6]
	//[weights] =[#,MAX,#,#,#,#]  <--THIS IS WHAT VALUES ARE STORED IN THE MATRIX
void dist_list_init(int num_vert, int dist[num_vert]){
	if(v1 = num_vert){
		dist[num_vert] = 0;
	}
	else{
		dist[num_vert] = INT_MAX;
	}
}
//do djikstra edge relaxing as you build your visited node graph. This is b/c you have
//to check the shortest path from every node  you visited not just the one currently at 
void edge_relax(int num_vert, vertexid_t v1, int dist[num_vert], int cost[num_vert][num_vert], int adj[num_vert][num_vert]){
	for( int i=0; i< num_vert; i++){
		if( (adj[v1][i]) == 1){
			if( dist[i] < (dist[v1] + cost[v1][i]) ){
				dist[i] = (dist[v1]+cost[v1][i]);
			}
		}
	}
}



	/*
	 * Figure out which attribute in the component edges schema you will
	 * use for your weight function
	 */



	/*
	 * Execute Dijkstra on the attribute you found for the specified
	 * component
	 */
//initialize a least weight vertex holder and each matrix/array used in previous
//funcitons that are needed for djikstras algorithm
int l_w_v = 0;
int flag[num_vert];
int dist[num_vert];
int cost[num_vert][num_vert];
int adj[num_vert][num_vert];

//procedure Dijkstra;
//{ Dijkstra computes the cost of the shortest paths
//from vertex 1 to every vertex of a directed graph }
//begin
//(1) S := {1};
	flag[v1] = 1;
	//printf("1 \n");
//(2) for i := 2 to n do
	edge_relax(num_vert,v1, dist, cost, adj);	
	//printf("2 \n");		
	for(int i=2; i<num_vert; i++){
		//printf("3 \n");
		//(3)     D[i] := C[1, i]; { initialize D }
		edge_relax(num_vert,v1, dist, cost, adj);		
		//(4)     for i := 1 to n-1 do begin
		//(5)         choose a vertex w in V-S such that D[w] is a minimum;
		//printf("4 \n");
		l_w_v = least_weight_calc(num_vert, flag,cost, adj);
		//(6)         add w to S; --- updating flag
		//printf("5 \n");		
		vertex_update(l_w_v,num_vert, flag);	
		//(7)         for each vertex v in V-S do begin
		//           D[v] := min(D[v], D[w] + C[w, v])
		//printf("6 \n");
		edge_relax(num_vert,l_w_v, dist, cost, adj);		
		//printf("%i\n", *flag);
		//printf("%i\n", *dist);
		//printf("%i\n", *cost);
		//printf("%i\n", *adj);

//                  { Path reconstruction }
//                  if (D[w] + C[w,v] < D[v])
//                      P[v]:= w
//             end
//          end
//end; { Dijkstra }
	}
	//printf("7 \n");
	*total_weight = dist[v2-1];
	printf("%i\n", *total_weight);
	/* Change this as needed */
	return (-1);
}


/*
procedure Dijkstra;
{ Dijkstra computes the cost of the shortest paths
from vertex 1 to every vertex of a directed graph }
begin
(1) S := {1};
(2) for i := 2 to n do
(3)     D[i] := C[1, i]; { initialize D }
(4)     for i := 1 to n-1 do begin
(5)         choose a vertex w in V-S such that D[w] is a minimum;
(6)         add w to S;
(7)         for each vertex v in V-S do begin
           D[v] := min(D[v], D[w] + C[w, v])
                  { Path reconstruction }
                  if (D[w] + C[w,v] < D[v])
                      P[v]:= w
              end
          end
end; { Dijkstra }

*/


/*

*/
