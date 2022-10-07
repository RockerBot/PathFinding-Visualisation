#include <math.h>
#include <string.h>

#define VIZ_COLS 60
#define VIZ_ROWS 60
// int VIZ_COLS=100;
// int VIZ_ROWS=60;
#define _COLS (VIZ_COLS+2)
#define _ROWS (VIZ_ROWS+2)
#define h_at(_pt) ((algo_flag==1)?(abs(end.y-(_pt)/_COLS)+abs(end.x-(_pt)%_COLS)):0)
#define wt_at(_pt) ((algo_flag!=2 && draw_grid[0][_pt]&DRAW_WT)?WT_WT:1)

typedef struct {
  int x;
  int y;
} viz_node;

typedef struct {
  int *draw_grid;//[_ROWS][_COLS]
  viz_node strt;
  viz_node end;
  GtkWidget *widget;
}algo_packet;

typedef struct {
  double red;
  double green;
  double blue;
}colour_codes;

char DRAW_NONE = 0,
     DRAW_WALL = 1,
     DRAW_WT = 2,
     DRAW_STRT = 4,
     DRAW_END = 8,
     DRAW_TOUCH = 16,
     DRAW_VISIT = 32,
     DRAW_PATH = 64;

double maze_speed=1.0, speed=1.0;
int sparcity=2, WT_WT=15;

void heap_ify(int heap[], int len, int cost[_ROWS][_COLS], int i, viz_node end, int algo_flag){
  if (len > 1){
    int smallest = i;
    int l = 2*i+1;
    int r = 2*i+2;
    if (l<len && cost[0][heap[l]]+h_at(heap[l]) < cost[0][heap[smallest]]+h_at(heap[smallest]))smallest = l;
    else if (l<len && cost[0][heap[l]]+h_at(heap[l]) == cost[0][heap[smallest]]+h_at(heap[smallest]) && h_at(heap[l]) < h_at(heap[smallest]))smallest = l;
    if (r<len && cost[0][heap[r]]+h_at(heap[r]) < cost[0][heap[smallest]]+h_at(heap[smallest]))smallest = r;
    else if (r<len && cost[0][heap[r]]+h_at(heap[r]) == cost[0][heap[smallest]]+h_at(heap[smallest]) && h_at(heap[r]) < h_at(heap[smallest]))smallest = r;
    if (smallest != i){
      int temp = heap[smallest];
      heap[smallest] = heap[i];
      heap[i] = temp;
      heap_ify(heap, len, cost, smallest, end, algo_flag);
    }
  }
}
void heap_add(int heap[], int len, int cost[_ROWS][_COLS], int val, viz_node end, int algo_flag){
  if(len==0)heap[0] = val; //val is position
  else{
    heap[len] = val; //val is position
    len++;
    for (int i = len/2-1;i>=0;i--)heap_ify(heap, len, cost, i, end, algo_flag);
  }
}
int heap_pop(int heap[], int len, int cost[_ROWS][_COLS], viz_node end, int algo_flag){
  int out = heap[0];
  heap[0]=heap[--len];
  for(int i=len/2-1;i>=0;i--)heap_ify(heap, len, cost, i, end, algo_flag);
  return out;//pos
}

void dijk_astr_bfs(int draw_grid[_ROWS][_COLS], viz_node strt, viz_node end, GtkWidget *widget,int algo_flag){
  int WT_WALL = -1, WT_NONE = 9999999;
  int cost[_ROWS][_COLS] = {};
  int prev[_ROWS][_COLS] = {};
  int pq[(_ROWS)*(_COLS)] = {};

  for(int i = 0;i<_ROWS;i++) for(int j = 0;j<_COLS;j++) cost[i][j] = WT_NONE;
  cost[strt.y][strt.x] = 0;

  int pq_len = 1;
  pq[0] = strt.x+strt.y*(_COLS);


  int NBRS[]={-_COLS,-1,_COLS,1};

  while (pq_len) {
    int curr = heap_pop(pq, pq_len, cost, end, algo_flag);pq_len--;
    if (curr==end.x+end.y*(_COLS)){
      while(curr!=strt.x+strt.y*(_COLS)){
        draw_grid[0][curr] |= DRAW_PATH;
        curr = prev[0][curr];
      }
      gtk_widget_queue_draw(widget); break;
    }

    for(int i=0;i<4;i++){
      if(draw_grid[0][curr+NBRS[i]]!=DRAW_WALL && cost[0][curr]+wt_at(curr+NBRS[i])<cost[0][curr+NBRS[i]]){
        cost[0][curr+NBRS[i]] = cost[0][curr]+wt_at(curr+NBRS[i]);      
        heap_add(pq, pq_len, cost, curr+NBRS[i], end, algo_flag);pq_len++;
        prev[0][curr+NBRS[i]] = curr;
        draw_grid[0][curr+NBRS[i]] |= DRAW_TOUCH;
      }
    }

    draw_grid[0][curr] |= DRAW_VISIT;
    usleep((int)(200000/speed));//10000
    gtk_widget_queue_draw(widget);
  }

}

void divide(int grid[_ROWS][_COLS], int strt_x, int strt_y, int end_x, int end_y, GtkWidget *widget){
  int width=(abs(end_x-strt_x)+1);
  int height=(abs(end_y-strt_y)+1);
  if(width<=sparcity || height<=sparcity) return;

  int is_horiz = (width==height)?(rand()>RAND_MAX/2):(width<height);
  int wall,gap;
  if(is_horiz){//draw wall horizontal
    wall = (strt_y+rand()%height)/2*2;
    if(wall<strt_y)wall+=2;
    for (int i = strt_x; i <=end_x; ++i)grid[wall][i]=DRAW_WALL;

    gap=(strt_x+rand()%width)/2*2-1;
    if(gap<strt_x)gap+=2;
    grid[wall][gap]=DRAW_NONE;

    usleep((int)(500000/maze_speed));
    gtk_widget_queue_draw(widget);
    divide(grid, strt_x, strt_y, end_x, wall-1, widget);
    divide(grid, strt_x, wall+1, end_x, end_y, widget);
  }else{
    wall = (strt_x+rand()%width)/2*2;
    if(wall<strt_x)wall+=2;
    for (int i = strt_y; i <=end_y; ++i)grid[i][wall]=DRAW_WALL;

    gap=(strt_y+rand()%height)/2*2-1;
    if(gap<strt_y)gap+=2;
    grid[gap][wall]=DRAW_NONE;

    usleep((int)(500000/maze_speed));
    gtk_widget_queue_draw(widget);
    divide(grid, strt_x, strt_y, wall-1, end_y, widget);
    divide(grid, wall+1, strt_y, end_x, end_y, widget);
  }
}