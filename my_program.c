#include <gtk/gtk.h>
#include <pthread.h>
#include <cairo.h>

#include <unistd.h>
#include "general.h"
#define tlBrOfst 55
#define LEFT_CLICK 1
#define RIGHT_CLICK 3
#define CLICK_UP 0
#define COLOUR(a,b) cairo_set_source_rgb(a, b.red, b.green, b.blue)

viz_node strt_node={3,3}, end_node={17,17};
colour_codes NONE_CLR ={1.00, 1.00, 1.00},//255,255,255
             STRT_CLR ={0.25, 0.63, 0.09},//64,161,23
             END_CLR  ={1.00, 0.00, 0.00},//255,0,0
             WALL_CLR ={0.00, 0.00, 0.15},//0,0,38
             WT_CLR   ={0.00, 0.00, 0.00},//0,0,38
             VISIT_CLR={0.37, 0.43, 0.50},//94,110,128
             TOUCH_CLR={0.00, 0.54, 0.54},//0,138,138
             PATH_CLR ={1.00, 1.00, 0.00},//255,255,0
             BRDR_CLR ={0.11, 0.11, 0.11};//28,28,28

int ALGOS_AStr = 1, ALGOS_Dijk = 2, ALGOS_BFS = 4, ALGOS_DFS = 8;
int NODE_STR_ND= 1, NODE_WALL = 2, NODE_WT = 4;

int algo_flag = 1;
int node_flag = 1;
int has_border = 1;
int btn_pressed = CLICK_UP;
int simulating = FALSE;
int BLK_sz = 25;
int width = 25*VIZ_COLS, height = 25*VIZ_ROWS+tlBrOfst;
int node_grid[VIZ_ROWS+2][VIZ_COLS+2]={};

char * config_file="visualizer_config.txt";

static gboolean mouse_moved(GtkWidget *,GdkEvent *, gpointer);
static gboolean button_up (GtkWidget *, GdkEventButton *, gpointer);
static gboolean button_down (GtkWidget *, GdkEventButton *, gpointer);
static gboolean on_draw_event (GtkWidget *, cairo_t *, gpointer);
static void draw_grid (cairo_t *);
static void draw_nodes (cairo_t *);
static void set_algo (GtkMenuItem *, gpointer);
static void toogle_node (GtkToggleToolButton *, gpointer);
static void *maze_div(void *);
static void *run_astr (void *);
static void *run_dijk (void *);
static void *run_bfs (void *);
static void *run_dfs (void *);
static void run_sim (GtkToolButton*, gpointer);
static void clear_screen (GtkToolButton*, gpointer);
static void gen_maze (GtkToolButton*, gpointer);
static void load_presets (GtkToolButton*, gpointer);
static void set_colours();

GtkWidget *window;
GtkWidget *box1;
GtkWidget *darea;
GtkWidget *tl_bar;
GtkWidget *algo_mnu_itm;
GtkWidget *algo_mnu;
GtkWidget *aStr_mnu_itm;
GtkWidget *dijk_mnu_itm;
GtkWidget *dfs_mnu_itm;
GtkWidget *bfs_mnu_itm;
GtkWidget *algo_mnu_bar;

GtkToolItem* Algo_baritm;
GtkToolItem* sim_barbtn;
GtkToolItem* clear_barbtn;
GtkToolItem* gen_mz_barbtn;
GtkToolItem* preset_barbtn;
GtkToolButton *strt_nd_barbtn;
GtkToolButton *wall_barbtn;
GtkToolButton *wt_barbtn;

static gboolean mouse_moved(GtkWidget *widget,GdkEvent *event, gpointer user_data) {
  if (event->type==GDK_MOTION_NOTIFY && !simulating && btn_pressed) {
    GdkEventMotion* e=(GdkEventMotion*)event;
    int x = ((int)e->x)/BLK_sz+1,
        y = ((int)(e->y))/BLK_sz+1;
    if(node_flag&NODE_STR_ND){

      if(btn_pressed==LEFT_CLICK && (end_node.x!=x || end_node.y!=y)){
        node_grid[strt_node.y][strt_node.x]=DRAW_NONE;
        strt_node.x=x;
        strt_node.y=y;
        node_grid[y][x]=DRAW_STRT;
      }else if(btn_pressed==RIGHT_CLICK && (strt_node.x!=x || strt_node.y!=y)){
        node_grid[end_node.y][end_node.x]=DRAW_NONE;
        end_node.x=x;
        end_node.y=y;
        node_grid[y][x]=DRAW_END;
      }

    }else if((strt_node.x!=x || strt_node.y!=y) && (end_node.x!=x || end_node.y!=y)){
      if(node_flag&NODE_WALL){
        if(btn_pressed==LEFT_CLICK)node_grid[y][x]=DRAW_WALL;
        else if(btn_pressed==RIGHT_CLICK)node_grid[y][x]=DRAW_NONE;
      }else if(node_flag&NODE_WT){
        if(btn_pressed==LEFT_CLICK)node_grid[y][x]|=DRAW_WT;
        else if(btn_pressed==RIGHT_CLICK)node_grid[y][x]=DRAW_NONE;
      }
    }
    gtk_widget_queue_draw(widget);
  }
}
static gboolean button_up(GtkWidget *widget, GdkEventButton *event, gpointer user_data){
  btn_pressed=CLICK_UP;
}
static gboolean button_down(GtkWidget *widget, GdkEventButton *event, gpointer user_data){
  int x = ((int)event->x)/BLK_sz+1,
      y = ((int)(event->y))/BLK_sz+1;
  btn_pressed=event->button;
  if(!simulating){

    if(node_flag&NODE_STR_ND){

      if(event->button==LEFT_CLICK && (end_node.x!=x || end_node.y!=y)){
        strt_node.x=x;
        strt_node.y=y;
        node_grid[y][x]=DRAW_STRT;
      }else if(event->button==RIGHT_CLICK && (strt_node.x!=x || strt_node.y!=y)){
        end_node.x=x;
        end_node.y=y;
        node_grid[y][x]=DRAW_END;
      }

    }else if((strt_node.x!=x || strt_node.y!=y) && (end_node.x!=x || end_node.y!=y)){
      if(node_flag&NODE_WALL){
        if(event->button==LEFT_CLICK)node_grid[y][x]=DRAW_WALL;
        else if(event->button==RIGHT_CLICK)node_grid[y][x]=DRAW_NONE;
      }else if(node_flag&NODE_WT){
        if(event->button==LEFT_CLICK)node_grid[y][x]|=DRAW_WT;
        else if(event->button==RIGHT_CLICK)node_grid[y][x]=DRAW_NONE;
      }
    }
  }
  gtk_widget_queue_draw(widget);
  return TRUE;
}

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data){
  GtkWidget *win = gtk_widget_get_toplevel(widget);
  gtk_window_get_size(GTK_WINDOW(win), &width, &height);
  height-=tlBrOfst;
  BLK_sz = (width<height)?width/VIZ_COLS:height/VIZ_ROWS;
  COLOUR(cr,WALL_CLR);
  cairo_rectangle(cr,0,0,BLK_sz*(VIZ_COLS+1),BLK_sz*(VIZ_ROWS+1));
  cairo_fill(cr);
  COLOUR(cr,NONE_CLR);
  cairo_rectangle(cr,0,0,BLK_sz*VIZ_COLS,BLK_sz*VIZ_ROWS);
  cairo_fill(cr);
  draw_nodes(cr);
  if(has_border)draw_grid(cr);
  return FALSE;
}
static void draw_grid(cairo_t *cr){
  COLOUR(cr,BRDR_CLR);
  for(int i=0;i<=VIZ_COLS;i++){
    cairo_move_to(cr,i*BLK_sz,0);
    cairo_line_to(cr,i*BLK_sz,VIZ_ROWS*BLK_sz);
  }
  for(int i=0;i<=VIZ_ROWS;i++){
    cairo_move_to(cr,0,i*BLK_sz);
    cairo_line_to(cr,VIZ_COLS*BLK_sz,i*BLK_sz);
  }
  cairo_stroke(cr);
}
static void draw_nodes(cairo_t *cr){
  for(int i=1;i<=VIZ_ROWS;i++){
    for(int j=1;j<=VIZ_COLS;j++){
      if(node_grid[i][j]==DRAW_NONE)continue;

      if(i==strt_node.y && j==strt_node.x)COLOUR(cr,STRT_CLR);
      else if(i==end_node.y && j==end_node.x)COLOUR(cr,END_CLR);
      else if(node_grid[i][j]&DRAW_WALL)COLOUR(cr,WALL_CLR);
      else if(node_grid[i][j]&DRAW_PATH)COLOUR(cr,PATH_CLR);
      else if(node_grid[i][j]&DRAW_VISIT)COLOUR(cr,VISIT_CLR);
      else if(node_grid[i][j]&DRAW_TOUCH)COLOUR(cr,TOUCH_CLR);
      else COLOUR(cr,NONE_CLR);
      cairo_rectangle(cr, (j-1)*BLK_sz, (i-1)*BLK_sz , BLK_sz, BLK_sz);
      cairo_fill(cr);

      if(algo_flag == ALGOS_BFS)continue;
      if(node_grid[i][j]&DRAW_WT){
        COLOUR(cr,WT_CLR);
        cairo_rectangle(cr, (j-1)*BLK_sz+BLK_sz/4, (i-1)*BLK_sz+BLK_sz/4 , BLK_sz/2, BLK_sz/2);
        cairo_fill(cr);
      }
    }
  }
  cairo_stroke(cr);
}
static void clear_screen(GtkToolButton* self, gpointer user_data){
  int flag=0;
  for(int i=1;i<=VIZ_ROWS;i++){
    for(int j=1;j<=VIZ_COLS;j++){
      if(node_grid[i][j]&DRAW_VISIT ||node_grid[i][j]&DRAW_TOUCH||node_grid[i][j]&DRAW_PATH){flag|=2;break;}
      else if(node_grid[i][j]&DRAW_WT ||node_grid[i][j]&DRAW_WALL)flag|=1;
    }
  }
  if(flag>1){
    for(int i=1;i<=VIZ_ROWS;i++){
      for(int j=1;j<=VIZ_COLS;j++){
        if(node_grid[i][j]&DRAW_WALL)node_grid[i][j]=DRAW_WALL;
        else if(node_grid[i][j]&DRAW_WT)node_grid[i][j]=DRAW_WT;
        else if(node_grid[i][j]&DRAW_STRT)node_grid[i][j]=DRAW_STRT;
        else if(node_grid[i][j]&DRAW_END)node_grid[i][j]=DRAW_END;
        else node_grid[i][j] = DRAW_NONE;
      }
    }
  }else if(flag==1){
    for(int i=1;i<=VIZ_ROWS;i++){
      for(int j=1;j<=VIZ_COLS;j++){
        if(node_grid[i][j]&DRAW_STRT)node_grid[i][j]=DRAW_STRT;
        else if(node_grid[i][j]&DRAW_END)node_grid[i][j]=DRAW_END;
        else node_grid[i][j] = DRAW_NONE;
      }
    }
  }
  gtk_widget_queue_draw(window);
}

static void gen_maze (GtkToolButton *self, gpointer user_data){
  if(simulating)return;
  simulating=TRUE;

  pthread_t thread_id1;
  pthread_create(&thread_id1, NULL, maze_div, NULL);
  
}
static void *maze_div(void *arg){
  for(int i=1;i<=VIZ_ROWS;i++){
    for(int j=1;j<=VIZ_COLS;j++){
      if(node_grid[i][j]&DRAW_STRT)node_grid[i][j]=DRAW_STRT;
      else if(node_grid[i][j]&DRAW_END)node_grid[i][j]=DRAW_END;
      else node_grid[i][j] = DRAW_NONE;
    }
  }
  divide(node_grid, 1,1, VIZ_COLS,VIZ_ROWS, window);
  node_grid[strt_node.y][strt_node.x]=DRAW_STRT;
  node_grid[end_node.y][end_node.x]=DRAW_END;
  simulating=FALSE;
}

static void set_algo(GtkMenuItem* self, gpointer user_data){
  algo_flag=*((int *)user_data);
  switch(algo_flag){
    case 1:gtk_menu_item_set_label ((GtkMenuItem *)algo_mnu_itm, "Algorithm: A*");break;
    case 2:gtk_menu_item_set_label ((GtkMenuItem *)algo_mnu_itm, "Algorithm: Dijkstra");break;
    case 4:gtk_menu_item_set_label ((GtkMenuItem *)algo_mnu_itm, "Algorithm: Breadth First Search");break;
    case 8:gtk_menu_item_set_label ((GtkMenuItem *)algo_mnu_itm, "Algorithm: Depth First Search");break;
  }
}
static void toogle_node(GtkToggleToolButton* self, gpointer user_data){
  switch(*(int *)user_data){
    case 1:{
      if(gtk_toggle_tool_button_get_active((GtkToggleToolButton*)strt_nd_barbtn)){
        node_flag=NODE_STR_ND;
        gtk_toggle_tool_button_set_active((GtkToggleToolButton*)wall_barbtn,FALSE);
        gtk_toggle_tool_button_set_active((GtkToggleToolButton*)wt_barbtn,FALSE);
      }break;
    }
    case 2:{
      if(gtk_toggle_tool_button_get_active((GtkToggleToolButton*)wall_barbtn)){
        node_flag=NODE_WALL;
        gtk_toggle_tool_button_set_active((GtkToggleToolButton*)strt_nd_barbtn,FALSE);
        gtk_toggle_tool_button_set_active((GtkToggleToolButton*)wt_barbtn,FALSE);
      }break;
    }
    case 4:{
      if(gtk_toggle_tool_button_get_active((GtkToggleToolButton*)wt_barbtn)){
        node_flag=NODE_WT;
        gtk_toggle_tool_button_set_active((GtkToggleToolButton*)strt_nd_barbtn,FALSE);
        gtk_toggle_tool_button_set_active((GtkToggleToolButton*)wall_barbtn,FALSE);
      }break;
    }
  }
  
}

static void *run_astr(void *arg){
  dijk_astr_bfs(node_grid, strt_node, end_node, window,1);
  simulating = FALSE;
}
static void *run_dijk(void *arg){
  dijk_astr_bfs(node_grid, strt_node, end_node, window,0);
  simulating = FALSE;
}
static void *run_bfs(void *arg){
  dijk_astr_bfs(node_grid, strt_node, end_node, window,2);
  simulating = FALSE;
}
static void *run_dfs(void *arg){ // TODO VIBHA or KRISHNA
  // dfs(node_grid, strt_node, end_node, window);
  simulating = FALSE;
}
static void run_sim(GtkToolButton* self, gpointer user_data){
  if(simulating)return;
  simulating=TRUE;
  for(int i=0;i<VIZ_ROWS+2;i++)node_grid[i][0] = node_grid[i][VIZ_COLS+1] = DRAW_WALL;
  for(int i=0;i<VIZ_COLS+2;i++)node_grid[0][i] = node_grid[VIZ_ROWS+1][i] = DRAW_WALL;
  // algo_packet info = {node_grid, strt_node, end_node, window};
  for(int i=1;i<VIZ_ROWS+1;i++){
    for(int j=1;j<VIZ_COLS+1;j++){
      if(node_grid[i][j]&DRAW_WALL)node_grid[i][j]=DRAW_WALL;
      else if(node_grid[i][j]&DRAW_WT)node_grid[i][j]=DRAW_WT;
      else if(node_grid[i][j]&DRAW_STRT)node_grid[i][j]=DRAW_STRT;
      else if(node_grid[i][j]&DRAW_END)node_grid[i][j]=DRAW_END;
      else node_grid[i][j] = DRAW_NONE;
    }
  }
  pthread_t thread_id1;
  switch(algo_flag){
    case 1: pthread_create(&thread_id1, NULL, run_astr, NULL); break;
    case 2: pthread_create(&thread_id1, NULL, run_dijk, NULL); break;
    case 4: pthread_create(&thread_id1, NULL, run_bfs, NULL); break;
    case 8: pthread_create(&thread_id1, NULL, run_dfs, NULL); break;
  }
}

static void load_presets(GtkToolButton* self, gpointer user_data){
  FILE *fp=fopen((char *)user_data,"r");
  if(fp==NULL){
    if((fp=fopen((char *)user_data,"w"))!=NULL){
      fprintf(fp,"%s","border:28,28,28\n");
      fprintf(fp,"%s","none:255,255,255\n");
      fprintf(fp,"%s","wall:0,0,38\n");
      fprintf(fp,"%s","weightt:0,0,38\n");
      fprintf(fp,"%s","start:64,161,23\n");
      fprintf(fp,"%s","end:255,0,0\n");
      fprintf(fp,"%s","visited:94,110,128\n");
      fprintf(fp,"%s","touched:0,138,138\n");
      fprintf(fp,"%s","path:255,255,0\n");
      fprintf(fp,"%s","has_border:1\n");
      fprintf(fp,"%s","maze_speed:100\n");
      fprintf(fp,"%s","speed:100\n");
      fprintf(fp,"%s","wt_val:15\n");
      fprintf(fp,"%s","sparcity:2\n");
      fprintf(fp,"%s","//#<filename.fileextension>\n");
      fclose(fp);
    }
    return;
  }
  char line[1000]={}, *val, *key;
  double r, g, b;
  while(fgets(line,1000,fp)!=NULL){
    key=strtok(line,":");
    if(key[0]=='#'){
      key=strtok(key,"\n");
      load_presets(self,&key[1]);
      continue;
    }else if(key[0]=='/' && key[1]=='/' || key[0]=='\n')continue;
    if((val=strtok(NULL, ","))!=NULL)r=atoi(val)/255.0;
    if((val=strtok(NULL, ","))!=NULL)g=atoi(val)/255.0;
    if((val=strtok(NULL, "\n"))!=NULL)b=atoi(val)/255.0;

    if(!strcmp(key,"border"))       BRDR_CLR.red  = r, BRDR_CLR.green  = g, BRDR_CLR.blue  = b;
    else if(!strcmp(key,"none"))    NONE_CLR.red  = r, NONE_CLR.green  = g, NONE_CLR.blue  = b;
    else if(!strcmp(key,"wall"))    WALL_CLR.red  = r, WALL_CLR.green  = g, WALL_CLR.blue  = b;
    else if(!strcmp(key,"weight"))  WT_CLR.red    = r, WT_CLR.green    = g, WT_CLR.blue    = b;
    else if(!strcmp(key,"start"))   STRT_CLR.red  = r, STRT_CLR.green  = g, STRT_CLR.blue  = b;
    else if(!strcmp(key,"end"))     END_CLR.red   = r, END_CLR.green   = g, END_CLR.blue   = b;
    else if(!strcmp(key,"visited")) VISIT_CLR.red = r, VISIT_CLR.green = g, VISIT_CLR.blue = b;
    else if(!strcmp(key,"touched")) TOUCH_CLR.red = r, TOUCH_CLR.green = g, TOUCH_CLR.blue = b;
    else if(!strcmp(key,"path"))    PATH_CLR.red  = r, PATH_CLR.green  = g, PATH_CLR.blue  = b;
    else if(!strcmp(key,"speed"))      speed      = (r*255);
    else if(!strcmp(key,"maze_speed")) maze_speed = (r*255);
    else if(!strcmp(key,"has_border")) has_border = (int)(r*255);
    else if(!strcmp(key,"wt_val"))     WT_WT      = (int)(((r*255)>0)?r*255:15);
    else if(!strcmp(key,"sparcity"))   sparcity   = (int)((2<=(r*255))?r*255:2);
  }
  gtk_widget_queue_draw(window);
  fclose(fp);
}

int main(int argc, char *argv[]){

  gtk_init(&argc, &argv);

  for(int i=0;i<VIZ_ROWS+2;i++)node_grid[i][0] = node_grid[i][VIZ_COLS+1] = DRAW_WALL;
  for(int i=0;i<VIZ_COLS+2;i++)node_grid[0][i] = node_grid[VIZ_ROWS+1][i] = DRAW_WALL;
  node_grid[strt_node.y][strt_node.x] = DRAW_STRT;
  node_grid[end_node.y][end_node.x] = DRAW_END;
  srand(time(0));

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), width, height); 
  gtk_window_set_title(GTK_WINDOW(window), "PathFinding");

  box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
  gtk_container_add(GTK_CONTAINER(window), box1);
  
  darea = gtk_drawing_area_new();


  tl_bar = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(tl_bar), GTK_TOOLBAR_BOTH);


  algo_mnu_bar=gtk_menu_bar_new();  
  
  algo_mnu_itm = gtk_menu_item_new_with_label("Algorithm: A*");
  gtk_menu_shell_append(GTK_MENU_SHELL(algo_mnu_bar), algo_mnu_itm);
  aStr_mnu_itm = gtk_menu_item_new_with_mnemonic("_A*");
  dijk_mnu_itm = gtk_menu_item_new_with_mnemonic("D_ijkstra");
  bfs_mnu_itm = gtk_menu_item_new_with_mnemonic("_Breadth First Search");
  dfs_mnu_itm = gtk_menu_item_new_with_mnemonic("_Depth First Search");
  algo_mnu = gtk_menu_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(algo_mnu), aStr_mnu_itm);
  gtk_menu_shell_append(GTK_MENU_SHELL(algo_mnu), dijk_mnu_itm);
  gtk_menu_shell_append(GTK_MENU_SHELL(algo_mnu), bfs_mnu_itm);
  gtk_menu_shell_append(GTK_MENU_SHELL(algo_mnu), dfs_mnu_itm);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(algo_mnu_itm), algo_mnu);

  Algo_baritm = gtk_tool_item_new();
  gtk_container_add(GTK_CONTAINER(Algo_baritm), algo_mnu_bar);
  gtk_toolbar_insert(GTK_TOOLBAR(tl_bar), Algo_baritm, -1);

  strt_nd_barbtn = (GtkToolButton *)gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(strt_nd_barbtn,"Start/End");
  // gtk_tool_button_set_icon_widget(strt_nd_barbtn,gtk_image_new_from_file ("my_imagefile.png"));
  gtk_toolbar_insert(GTK_TOOLBAR(tl_bar), (GtkToolItem *)strt_nd_barbtn, -1);

  wall_barbtn = (GtkToolButton *)gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(wall_barbtn,"Wall");
  // gtk_tool_button_set_icon_widget(wall_barbtn,gtk_image_new_from_file ("my_imagefile.png"));
  gtk_toolbar_insert(GTK_TOOLBAR(tl_bar), (GtkToolItem *)wall_barbtn, -1);

  wt_barbtn = (GtkToolButton *)gtk_toggle_tool_button_new();
  gtk_tool_button_set_label(wt_barbtn,"Weight");
  // gtk_tool_button_set_icon_widget(wt_barbtn,gtk_image_new_from_file ("my_imagefile.png"));
  gtk_toolbar_insert(GTK_TOOLBAR(tl_bar), (GtkToolItem *)wt_barbtn, -1);

  sim_barbtn = gtk_tool_button_new(NULL,"Visualize");
  gtk_toolbar_insert(GTK_TOOLBAR(tl_bar), sim_barbtn, -1);
  clear_barbtn = gtk_tool_button_new(NULL,"Clear");
  gtk_toolbar_insert(GTK_TOOLBAR(tl_bar), clear_barbtn, -1);
  gen_mz_barbtn = gtk_tool_button_new(NULL,"Generate MAZE");
  gtk_toolbar_insert(GTK_TOOLBAR(tl_bar), gen_mz_barbtn, -1);
  preset_barbtn = gtk_tool_button_new(NULL,"Load presets");
  gtk_toolbar_insert(GTK_TOOLBAR(tl_bar), preset_barbtn, -1);

  

  gtk_box_pack_start(GTK_BOX(box1),tl_bar,FALSE, FALSE,0);
  gtk_box_pack_start(GTK_BOX(box1),darea,TRUE, TRUE,0);

  g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event), NULL);
  g_signal_connect(G_OBJECT(darea),"motion-notify-event",G_CALLBACK(mouse_moved), NULL);
  g_signal_connect(window, "button-press-event", G_CALLBACK(button_down), NULL);
  g_signal_connect(window, "button-release-event", G_CALLBACK(button_up), NULL);
  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(G_OBJECT(aStr_mnu_itm), "activate",G_CALLBACK(set_algo), &ALGOS_AStr);
  g_signal_connect(G_OBJECT(dijk_mnu_itm), "activate",G_CALLBACK(set_algo), &ALGOS_Dijk);
  g_signal_connect(G_OBJECT(bfs_mnu_itm), "activate",G_CALLBACK(set_algo), &ALGOS_BFS);
  g_signal_connect(G_OBJECT(dfs_mnu_itm), "activate",G_CALLBACK(set_algo), &ALGOS_DFS);

  gtk_toggle_tool_button_set_active ((GtkToggleToolButton*)strt_nd_barbtn,TRUE);
  g_signal_connect(G_OBJECT(strt_nd_barbtn), "toggled",G_CALLBACK(toogle_node), &NODE_STR_ND);
  g_signal_connect(G_OBJECT(wall_barbtn), "toggled",G_CALLBACK(toogle_node), &NODE_WALL);
  g_signal_connect(G_OBJECT(wt_barbtn), "toggled",G_CALLBACK(toogle_node), &NODE_WT);
  g_signal_connect(G_OBJECT(sim_barbtn), "clicked", G_CALLBACK(run_sim), NULL);
  g_signal_connect(G_OBJECT(clear_barbtn), "clicked", G_CALLBACK(clear_screen), NULL);
  g_signal_connect(G_OBJECT(gen_mz_barbtn), "clicked", G_CALLBACK(gen_maze), NULL);
  g_signal_connect(G_OBJECT(preset_barbtn), "clicked", G_CALLBACK(load_presets), config_file);

  load_presets(NULL,config_file);

  gtk_widget_set_events(darea, GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}