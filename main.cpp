#include "pandaFramework.h"
#include "windowFramework.h"
#include "nodePath.h"
#include "clockObject.h"
#include "asyncTask.h"
#include "genericAsyncTask.h"
#include "bulletWorld.h"
#include "bulletPlaneShape.h"
#include "bulletSphereShape.h"
#include "bulletBoxShape.h"
#include "bulletCapsuleShape.h"
#include "bulletConeShape.h"
#include "bulletGhostNode.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "cardMaker.h"
#include "texture.h"
#include "texturePool.h"
#include "textureStage.h"
#include "ambientLight.h"
#include "directionalLight.h"
#include "pointLight.h"
#include "spotlight.h"
#include "audio.h"
#include "audioManager.h"
#include "audioSound.h"
#include "material.h"


using namespace std;

// each 10 step_point +1 ball
int step_point = 0;

// number left ball
int num_ball = 10;
int this_num_ball = -1;

// points
int total_point = 0;
int this_point = -1;

PT(TextNode) scoreText;
PT(TextNode) ballText;
PT(TextNode) infoText;

AsyncTask::DoneStatus updateScene(GenericAsyncTask *task, void *data);
AsyncTask::DoneStatus checkCollision(GenericAsyncTask *task, void *data);
AsyncTask::DoneStatus updateText(GenericAsyncTask *task, void *data);
void sysExit(const Event *eventPtr, void *dataPtr);
void infoTextHandler(const Event *eventPtr, void *dataPtr);
void keyHandler(const Event *eventPtr, void *dataPtr);
void debugModeHandler(const Event *eventPtr, void *dataPtr);
void initBall(NodePath *camera, BulletWorld *physics_world, WindowFramework *window, AudioSound *sound);
void initTable(BulletWorld *physics_world, WindowFramework *window);
void shot(const Event *eventPtr, void *dataPtr);
void loadCursor(NodePath &cursorFather, LVecBase3 pos);
void initGhostPlane(BulletWorld *physics_world, WindowFramework *window);
void initFloor(BulletWorld *physics_world, WindowFramework *window);
void initRoof(BulletWorld *physics_world, WindowFramework *window);
void initFrontWall(BulletWorld *physics_world, WindowFramework *window);
void initBackWall(BulletWorld *physics_world, WindowFramework *window);
void initLeftWall(BulletWorld *physics_world, WindowFramework *window);
void initRightWall(BulletWorld *physics_world, WindowFramework *window);
void initRoom(BulletWorld *physics_world, WindowFramework *window);
void initCube(BulletWorld *physics_world, WindowFramework *window, double posX, double posY, double posZ, double dim);
void initLight(WindowFramework *window);
void initCubePile(BulletWorld *physics_world, WindowFramework *window);

// scene updater
AsyncTask::DoneStatus updateScene(GenericAsyncTask *task, void *data)
{
    BulletWorld *physics_world = (BulletWorld*)data;
    PT(ClockObject) co = ClockObject::get_global_clock();
    physics_world->do_physics(co->get_dt(), 10, 1.0/180.0);
    //physics_world->do_physics(co->get_dt(), 10, 1.0/60.0);
    return AsyncTask :: DS_cont;
}

struct check_collision_data
{
    BulletWorld *physics_world;
    WindowFramework *window;
};
AsyncTask::DoneStatus checkCollision(GenericAsyncTask *task, void *data)
{
    struct check_collision_data* this_check_collision_data = (struct check_collision_data*) data;
    BulletWorld *physics_world = this_check_collision_data->physics_world;
    WindowFramework *window = this_check_collision_data->window;
    // ghost_check
    // i have only 1 ghost, so i can get_ghost(0) instead of cycling
    bool noBoxRemain = true;
    for(int i =0; i<physics_world->get_num_ghosts(); i++)
    {
        BulletGhostNode *node = physics_world->get_ghost(i);
        for(int j =0; j<physics_world->get_num_rigid_bodies(); j++)
        {
            BulletRigidBodyNode *node2 = physics_world->get_rigid_body(j);
            if(node2->get_name() == "Box")
            {
                BulletContactResult result =  physics_world->contact_test_pair(node,node2);
                if(node2->get_num_children() <2)
                {
                    noBoxRemain = false;
                    if(result.get_num_contacts() > 0)
                    {
                        //cout << "nuovo contatto" << endl;
                        node2->add_child(new PandaNode("empty"));
                        step_point++;
                        total_point++;
                    }
                }
            }
        }
    }

    if(step_point>10)
    {
        num_ball++;
        step_point = 0;
    }

    if(noBoxRemain)
    {
        num_ball += 5;
         // remove all stuff (?)
        /* not work 100%
        for(int i =0; i< window->get_render().get_num_children(); i++)
        {
            NodePath this_child = window->get_render().get_child(i);
            cout << this_child.get_name() << endl;
            if(this_child.get_name() == "Box")
            {
                this_child.remove_node();
            }
        }*/
        initCubePile(physics_world, window);
    }

    // add sound on finish ball or if add new ball ??
    return AsyncTask :: DS_cont;
}

AsyncTask::DoneStatus updateText(GenericAsyncTask *task, void *data)
{
    if(this_point==total_point && this_num_ball==num_ball) return AsyncTask :: DS_cont;

    this_point = total_point;
    this_num_ball = num_ball;

    std::ostringstream oss;
    oss << "Score: " << total_point;
    scoreText->set_text(oss.str());

    std::ostringstream oss2;
    oss2 << "Ball left: " << num_ball;
    ballText->set_text(oss2.str());
    return AsyncTask :: DS_cont;
}

// when called, generate exit(0) statement
void sysExit(const Event *eventPtr, void *dataPtr)
{
    int *exitState = (int *)dataPtr;
    exit((*exitState));
    return;
}

void infoTextHandler(const Event *eventPtr, void *dataPtr)
{
    if(infoText->get_text() == "")
    {
        infoText->set_text("KEY: aim->ARROWS, shot->SPACE, exit->ESC, debug mode->F1/F2/F3/F4, deactivate this menu->\"L\"");
    }
    else infoText->set_text("");
}

void keyHandler(const Event *eventPtr, void *dataPtr)
{
    NodePath * camera = (NodePath *)dataPtr;
    if(eventPtr->get_name() == "w") camera->set_pos(camera->get_pos()+LVecBase3(0.0,1.0,0.0));
    if(eventPtr->get_name() == "s") camera->set_pos(camera->get_pos()-LVecBase3(0.0,1.0,0.0));
    if(eventPtr->get_name() == "a") camera->set_pos(camera->get_pos()-LVecBase3(1.0,0.0,0.0));
    if(eventPtr->get_name() == "d") camera->set_pos(camera->get_pos()+LVecBase3(1.0,0.0,0.0));
    if(eventPtr->get_name() == "arrow_up") camera->set_hpr(LVecBase3(camera->get_h(),camera->get_p()+1.0,camera->get_r()));
    if(eventPtr->get_name() == "arrow_down") camera->set_hpr(LVecBase3(camera->get_h(),camera->get_p()-1.0,camera->get_r()));
    if(eventPtr->get_name() == "arrow_left") camera->set_hpr(LVecBase3(camera->get_h()+1.0,camera->get_p(),camera->get_r()));
    if(eventPtr->get_name() == "arrow_right") camera->set_hpr(LVecBase3(camera->get_h()-1.0,camera->get_p(),camera->get_r()));
}

void debugModeHandler(const Event *eventPtr, void *dataPtr)
{
    BulletDebugNode *debugNode = (BulletDebugNode *)dataPtr;
    static bool show_bounding_boxes_value = false;
    static bool show_constraints_value = false;
    static bool show_normals_value = false;
    static bool show_wireframe_value = false;
    if(eventPtr->get_name() == "f1")
    {
        show_bounding_boxes_value = !show_bounding_boxes_value;
        debugNode->show_bounding_boxes(show_bounding_boxes_value);
    }
    if(eventPtr->get_name() == "f2")
    {
        show_constraints_value = !show_constraints_value;
        debugNode->show_constraints(show_constraints_value);
    }
    if(eventPtr->get_name() == "f3")
    {
        show_normals_value = !show_normals_value;
        debugNode->show_normals(show_normals_value);
    }
    if(eventPtr->get_name() == "f4")
    {
        show_wireframe_value = !show_wireframe_value;
        debugNode->show_wireframe(show_wireframe_value);
    }
}

void initBall(NodePath *camera, BulletWorld *physics_world, WindowFramework *window, AudioSound *sound)
{
    if(num_ball <= 0 ) return;
    num_ball--;
  //Sphere shape -> Dynamic Bodies
  //PT(BulletSphereShape) sphere_shape;
  PT(BulletCapsuleShape) sphere_shape;
  PT(BulletRigidBodyNode) sphere_rigid_node;
  NodePath np_sphere, np_sphere_model;

  double bullet_dimension = 0.4;

  sphere_shape = new BulletCapsuleShape(0.43, 0.64);
  sphere_rigid_node = new BulletRigidBodyNode("Ball");

  sphere_rigid_node->set_mass(0.2);
  sphere_rigid_node->set_friction(10.0);
  sphere_rigid_node->add_shape(sphere_shape);

  double bullet_speed = 25.0;
  double bullet_rotarion_speed = 10.0;
  LVector3 vector_direction = camera->get_relative_vector(window->get_render(), camera->get_child(0).get_pos());
  LVector3 bullet_direction = bullet_speed*LVector3(vector_direction[0], -vector_direction[1], vector_direction[2]);
  LVector3 bullet_rotation = bullet_rotarion_speed*bullet_direction/bullet_speed;
  //cout << bullet_direction << endl;
  sphere_rigid_node->set_linear_velocity(bullet_direction);
  // angolar velocity after rotation to mantain right rotation
  sphere_rigid_node->set_angular_velocity(bullet_rotation);

  physics_world->attach_rigid_body(sphere_rigid_node);

  np_sphere = window->get_render().attach_new_node(sphere_rigid_node);
  np_sphere.set_pos(camera->get_pos());
  np_sphere.set_hpr(LVector3(0.0,90.0,0.0)+camera->get_hpr());

  //cout << "sparato da: " << camera->get_pos() << endl;

  //np_sphere_model = window->load_model(window->get_panda_framework()->get_models(), "smiley");
  np_sphere_model = window->load_model(window->get_panda_framework()->get_models(), "../SVDRV_as04/models/football/football.egg");
  np_sphere_model.set_hpr(0.0,90.0,180.0);

  np_sphere_model.reparent_to(np_sphere);

  // scale down all stuff if necessary
  np_sphere.set_scale(bullet_dimension);

  sound->play();
}

void initTable(BulletWorld *physics_world, WindowFramework *window)
{
  PT(BulletBoxShape) shape, shape2;
  PT(BulletRigidBodyNode) rigidNode;
  NodePath node, nodeModel;

  double scale_factor = 1.0;

  shape = new BulletBoxShape(LVecBase3(2.0,0.75,0.12));
  shape2 = new BulletBoxShape(LVecBase3(0.11,0.11,1.0));
  rigidNode = new BulletRigidBodyNode("Table");

  rigidNode->set_friction(10.0);
  rigidNode->add_shape(shape, TransformState::make_pos(LVecBase3(0.0, 0.0, 0.885)));
  rigidNode->add_shape(shape2, TransformState::make_pos(LVecBase3(-1.81, -0.56, 0.0)));
  rigidNode->add_shape(shape2, TransformState::make_pos(LVecBase3(1.81, -0.56, 0.0)));
  rigidNode->add_shape(shape2, TransformState::make_pos(LVecBase3(-1.81, 0.56, 0.0)));
  rigidNode->add_shape(shape2, TransformState::make_pos(LVecBase3(1.81, 0.56, 0.0)));

  physics_world->attach_rigid_body(rigidNode);

  node = window->get_render().attach_new_node(rigidNode);
  node.set_pos(0.0, 0.0, 1.0);

  nodeModel = window->load_model(window->get_panda_framework()->get_models(), "../SVDRV_as04/models/bigtable/BigTable.egg");
  nodeModel.set_pos(0.0, 0.0, -1.0);
  nodeModel.set_scale(0.61);

  nodeModel.reparent_to(node);

  // scale down all stuff if necessary
  node.set_scale(scale_factor);
}

struct shot_data
{
    NodePath *camera;
    BulletWorld *physics_world;
    WindowFramework *window;
    AudioSound *sound;
};

void shot(const Event *eventPtr, void *dataPtr)
{
    struct shot_data *this_shot_data = (struct shot_data*)dataPtr;
    initBall(this_shot_data->camera, this_shot_data->physics_world, this_shot_data->window, this_shot_data->sound);
}

// load a cursor and the relative texture
void loadCursor(NodePath &cursorFather, LVecBase3 pos)
{
    PT(BulletPlaneShape) cursorShape = new BulletPlaneShape(LVecBase3(0.0,0.0,0.0), 0.0);
    PT(BulletRigidBodyNode) cursorRigidNode = new BulletRigidBodyNode("cursor");
    cursorRigidNode->add_shape(cursorShape);
    NodePath cursor = cursorFather.attach_new_node(cursorRigidNode);
    cursor.set_pos(pos);

    CardMaker* cm = new CardMaker("pointer");
    cm->set_frame(-0.075, 0.075, -0.075, 0.075);

    NodePath cursorTexture = cursor.attach_new_node(cm->generate());

    PT(TextureStage) ts = new TextureStage("ts");
    ts->set_mode(TextureStage::M_modulate);
    PT(Texture) tex = TexturePool::get_global_ptr()->load_texture("../SVDRV_as04/texture/cursor.png");

    cursorTexture.set_texture(ts, tex);
    cursorTexture.set_transparency(TransparencyAttrib::M_alpha);
    cursor.show();
}

void initGhostPlane(BulletWorld *physics_world, WindowFramework *window)
{
  // ghost
  //PT(BulletPlaneShape) floor_shape = new BulletPlaneShape(LVecBase3(0.0,0.0,1.0),0.0);
  //PT(BulletRigidBodyNode) floor_rigid_node = new BulletRigidBodyNode("floor");
  //floor_rigid_node->add_shape(floor_shape);
  //physics_world->attach_rigid_body(floor_rigid_node);
  //NodePath ghostNode = window->get_render().attach_new_node(floor_rigid_node);

  PT(BulletPlaneShape) planeShape = new BulletPlaneShape(LVecBase3(0.0,0.0,1.0),0.0);
  PT(BulletGhostNode) ghostPlane = new BulletGhostNode("ghostPlane");
  ghostPlane->add_shape(planeShape);
  physics_world->attach_ghost(ghostPlane);

  NodePath ghostNode = window->get_render().attach_new_node(ghostPlane);
  ghostNode.set_pos(0, 0, 0.9);

  // show a trasparent plane
  //LPlane *p = new LPlane(LVector3(0, 0, 1), LPoint3(0, 0, 0));
  //CollisionNode *cgNode = new CollisionNode("Ghost");
  //cgNode->add_solid(new CollisionPlane(*p));
  //NodePath ghostNodeModel = window->get_render().attach_new_node(cgNode);
  //ghostNodeModel.reparent_to(ghostNode);
}


void initFloor(BulletWorld *physics_world, WindowFramework *window)
{
  // floor
  PT(BulletPlaneShape) floor_shape = new BulletPlaneShape(LVecBase3(0.0,0.0,1.0),0.0);
  PT(BulletRigidBodyNode) floor_rigid_node = new BulletRigidBodyNode("floor");

  floor_rigid_node->set_friction(10.0);
  floor_rigid_node->add_shape(floor_shape);

  physics_world->attach_rigid_body(floor_rigid_node);

  NodePath np_ground =  window->get_render().attach_new_node(floor_rigid_node);
  np_ground.set_pos(0, 0, 0);

  CardMaker *cm = new CardMaker("ground");
  cm->set_frame(-10, 10, -10, 10);

  NodePath np_ground_tex = window->get_render().attach_new_node(cm->generate());
  np_ground_tex.reparent_to(np_ground);

  PT(TextureStage) ts = new TextureStage("ts");
  ts->set_mode(TextureStage::M_modulate);
  PT(Texture) tex = TexturePool::get_global_ptr()->load_texture("../SVDRV_as04/texture/floor.jpg");

  np_ground_tex.set_p(270);
  np_ground_tex.set_tex_scale(ts, 10, 10);
  np_ground.set_texture(ts, tex);
}

void initRoof(BulletWorld *physics_world, WindowFramework *window)
{
  // roof
  PT(BulletPlaneShape) floor_shape = new BulletPlaneShape(LVecBase3(0.0,0.0,-1.0),0.0);
  PT(BulletRigidBodyNode) floor_rigid_node = new BulletRigidBodyNode("roof");

  floor_rigid_node->set_friction(10.0);
  floor_rigid_node->add_shape(floor_shape);


  NodePath np_ground =  window->get_render().attach_new_node(floor_rigid_node);
  np_ground.set_pos(0, 0, 10.0);

  physics_world->attach_rigid_body(floor_rigid_node);

  CardMaker *cm = new CardMaker("ground");
  cm->set_frame(-10, 10, -10, 10);

  NodePath np_ground_tex = window->get_render().attach_new_node(cm->generate());
  np_ground_tex.reparent_to(np_ground);

  PT(TextureStage) ts = new TextureStage("ts");
  ts->set_mode(TextureStage::M_modulate);
  PT(Texture) tex = TexturePool::get_global_ptr()->load_texture("../SVDRV_as04/texture/roof.jpg");

  np_ground_tex.set_p(90);
  np_ground_tex.set_tex_scale(ts, 1, 1);
  np_ground.set_texture(ts, tex);
}

void initFrontWall(BulletWorld *physics_world, WindowFramework *window)
{
  // front wall
  PT(BulletPlaneShape) floor_shape = new BulletPlaneShape(LVecBase3(0.0,-1.0,0.0),0.0);
  PT(BulletRigidBodyNode) floor_rigid_node = new BulletRigidBodyNode("frontWall");

  floor_rigid_node->set_friction(10.0);
  floor_rigid_node->add_shape(floor_shape);


  NodePath np_ground =  window->get_render().attach_new_node(floor_rigid_node);
  np_ground.set_pos(0, 10.0, 5);

  physics_world->attach_rigid_body(floor_rigid_node);

  CardMaker *cm = new CardMaker("ground");
  cm->set_frame(-10, 10, -5, 5);

  NodePath np_ground_tex = window->get_render().attach_new_node(cm->generate());
  np_ground_tex.reparent_to(np_ground);

  PT(TextureStage) ts = new TextureStage("ts");
  ts->set_mode(TextureStage::M_modulate);
  PT(Texture) tex = TexturePool::get_global_ptr()->load_texture("../SVDRV_as04/texture/ .jpg");

  //np_ground_tex.set_p(90);
  np_ground_tex.set_tex_scale(ts, 1, 1);
  np_ground.set_texture(ts, tex);
}


void initBackWall(BulletWorld *physics_world, WindowFramework *window)
{
  // back wall
  PT(BulletPlaneShape) floor_shape = new BulletPlaneShape(LVecBase3(0.0,1.0,0.0),0.0);
  PT(BulletRigidBodyNode) floor_rigid_node = new BulletRigidBodyNode("backWall");

  floor_rigid_node->set_friction(10.0);
  floor_rigid_node->add_shape(floor_shape);


  NodePath np_ground =  window->get_render().attach_new_node(floor_rigid_node);
  np_ground.set_pos(0, -10.0, 5.0);

  physics_world->attach_rigid_body(floor_rigid_node);

  CardMaker *cm = new CardMaker("ground");
  cm->set_frame(-10, 10, -5, 5);

  NodePath np_ground_tex = window->get_render().attach_new_node(cm->generate());
  np_ground_tex.reparent_to(np_ground);

  PT(TextureStage) ts = new TextureStage("ts");
  ts->set_mode(TextureStage::M_modulate);
  PT(Texture) tex = TexturePool::get_global_ptr()->load_texture("../SVDRV_as04/texture/wall.jpg");

  np_ground_tex.set_h(180);
  np_ground_tex.set_tex_scale(ts, 1, 1);
  np_ground.set_texture(ts, tex);
}

void initLeftWall(BulletWorld *physics_world, WindowFramework *window)
{
  // left wall
  PT(BulletPlaneShape) floor_shape = new BulletPlaneShape(LVecBase3(1.0,0.0,0.0),0.0);
  PT(BulletRigidBodyNode) floor_rigid_node = new BulletRigidBodyNode("leftWall");

  floor_rigid_node->set_friction(10.0);
  floor_rigid_node->add_shape(floor_shape);


  NodePath np_ground =  window->get_render().attach_new_node(floor_rigid_node);
  np_ground.set_pos(-10.0, 0.0, 5.0);

  physics_world->attach_rigid_body(floor_rigid_node);

  CardMaker *cm = new CardMaker("ground");
  cm->set_frame(-10, 10, -5, 5);

  NodePath np_ground_tex = window->get_render().attach_new_node(cm->generate());
  np_ground_tex.reparent_to(np_ground);

  PT(TextureStage) ts = new TextureStage("ts");
  ts->set_mode(TextureStage::M_modulate);
  PT(Texture) tex = TexturePool::get_global_ptr()->load_texture("../SVDRV_as04/texture/wall.jpg");

  np_ground_tex.set_h(90);
  np_ground_tex.set_tex_scale(ts, 1, 1);
  np_ground.set_texture(ts, tex);
}
void initRightWall(BulletWorld *physics_world, WindowFramework *window)
{
  // right wall
  PT(BulletPlaneShape) floor_shape = new BulletPlaneShape(LVecBase3(-1.0,0.0,0.0),0.0);
  PT(BulletRigidBodyNode) floor_rigid_node = new BulletRigidBodyNode("rightWall");

  floor_rigid_node->set_friction(10.0);
  floor_rigid_node->add_shape(floor_shape);


  NodePath np_ground =  window->get_render().attach_new_node(floor_rigid_node);
  np_ground.set_pos(10.0, 0.0, 5.0);

  physics_world->attach_rigid_body(floor_rigid_node);

  CardMaker *cm = new CardMaker("ground");
  cm->set_frame(-10, 10, -5, 5);

  NodePath np_ground_tex = window->get_render().attach_new_node(cm->generate());
  np_ground_tex.reparent_to(np_ground);

  PT(TextureStage) ts = new TextureStage("ts");
  ts->set_mode(TextureStage::M_modulate);
  PT(Texture) tex = TexturePool::get_global_ptr()->load_texture("../SVDRV_as04/texture/wall.jpg");

  np_ground_tex.set_h(-90);
  np_ground_tex.set_tex_scale(ts, 1, 1);
  np_ground.set_texture(ts, tex);
}

void initRoom(BulletWorld *physics_world, WindowFramework *window)
{
    initFloor(physics_world,window);
    initRoof(physics_world,window);
    initFrontWall(physics_world,window);
    initBackWall(physics_world,window);
    initLeftWall(physics_world,window);
    initRightWall(physics_world,window);
}

void initCube(BulletWorld *physics_world, WindowFramework *window, double posX, double posY, double posZ, double dim)
{
  PT(BulletBoxShape) box_shape;
  PT(BulletRigidBodyNode) box_rigid_node;
  NodePath np_box, np_box_model;

  box_shape = new BulletBoxShape(LVecBase3(dim,dim,dim));
  box_rigid_node = new BulletRigidBodyNode("Box");

  box_rigid_node->set_mass(0.2);
  box_rigid_node->set_friction(10.0);
  box_rigid_node->add_shape(box_shape);

  physics_world->attach_rigid_body(box_rigid_node);

  np_box = window->get_render().attach_new_node(box_rigid_node);
  //np_box.set_pos_hpr(2, 0, 10, 45, 45, 45);
  np_box.set_pos(posX, posY, posZ);

  //Create a plane and a its CollisionPlane (to connect to cgNode)
  //PT(CollisionNode) cgNode = new CollisionNode("TrasparentBox");
  //cgNode->add_solid(new CollisionBox(LPoint3(0.0,0.0,0.0),dim,dim,dim));
  //np_box_model = window->get_render().attach_new_node(cgNode);
  np_box_model = window->load_model(window->get_panda_framework()->get_models(), "../SVDRV_as04/models/buildingblock/BuildingBlock.egg");
  np_box_model.set_pos(-0.0, -0.0, -0.1);
  np_box_model.set_scale(1.25);
  np_box_model.reparent_to(np_box);
  np_box_model.show();

  //random rotation of cubes
  // remember to add in main srand(time(NULL)) or equivalent;
  int rand_h = rand()%4;
  int rand_p = rand()%4;
  int rand_r = rand()%4;
  np_box.set_hpr(90.0*rand_h, 90.0*rand_p, 90.0*rand_r);

}

void initLight(WindowFramework *window)
{
    // ambient light 01
    PT(AmbientLight) ambientLight = new AmbientLight("ambientLight");
    ambientLight->set_color(LVecBase4(0.4, 0.4, 0.4, 1));
    NodePath ambientLightNode = window->get_render().attach_new_node(ambientLight);
    window->get_render().set_light(ambientLightNode);

    // directional light 02
    PT(DirectionalLight) directionalLight = new DirectionalLight("directionalLight");
    directionalLight->set_color(LVecBase4(0.8, 0.8, 0.8, 1));
    NodePath directionalLightNode = window->get_render().attach_new_node(directionalLight);
    // This light is facing backwards, towards the camera.
    directionalLightNode.set_hpr(180, -20, 0);
    window->get_render().set_light(directionalLightNode);

    // directional light 01
    directionalLight = new DirectionalLight("directionalLight");
    directionalLight->set_color(LVecBase4(0.8, 0.8, 0.8, 1));
    directionalLightNode = window->get_render().attach_new_node(directionalLight);
    // This light is facing forwards, away from the camera.
    directionalLightNode.set_hpr(0, -20, 0);
    window->get_render().set_light(directionalLightNode);
}

void initShadow(WindowFramework * window)
{

}

void initCubePile(BulletWorld *physics_world, WindowFramework *window)
{
    int rowNumber = 10;
    double dimension = 0.10;
    for(int i = 0; i< rowNumber; i++)
    {
        for(int j=rowNumber-i; j> 0; j--)
        {
            initCube(physics_world, window, (-1.0*(rowNumber+1)*dimension)+2.0*dimension*j+(i*dimension), 0.0, (2.0*dimension*(i+0.5))+2.0, dimension);
        }
    }

}

int main(int argc, char *argv[])
{
    PandaFramework framework;
    PT(WindowFramework) window;
    PT(BulletWorld) physics_world;
    PT(AsyncTaskManager) task_mgr = AsyncTaskManager::get_global_ptr();
    NodePath camera;

    framework.open_framework(argc, argv);
    framework.set_window_title("SVDRV_as04");

    window = framework.open_window();
    window->enable_keyboard();
    // use default mouse movement => development only
    //window->setup_trackball();

    camera = window->get_camera_group();
    loadCursor(camera, LVecBase3(0.0,0.0,0.0));
    camera.set_pos(0, -9.5, 3.0);
    // first children is camera cursor
    camera.get_child(0).set_pos(LVecBase3(0.0,-3.0,0.0)); // camera cursor
    //camera.look_at(camera.get_child(0).get_pos(window->get_render()));

    physics_world = new BulletWorld();
    physics_world->set_gravity(0,0,-9.8);

    // debug node
    PT(BulletDebugNode) bullet_dbg_node;
    bullet_dbg_node = new BulletDebugNode("Debug");
    bullet_dbg_node->show_bounding_boxes(false);
    bullet_dbg_node->show_constraints(false);
    bullet_dbg_node->show_normals(false);
    bullet_dbg_node->show_wireframe(false);
    NodePath np_dbg_node = window->get_render().attach_new_node(bullet_dbg_node);
    np_dbg_node.show();
    physics_world->set_debug_node(bullet_dbg_node);
    // end debug node

    initRoom(physics_world, window);
    initGhostPlane(physics_world, window);
    initTable(physics_world, window);

    // srand shuffle rand numbers
    srand(ClockObject::get_global_clock()->get_real_time());

    //initCube(physics_world, window, 0.0, 0.0, 20.0, 0.10);

    initCubePile(physics_world, window);

    // text
    scoreText = new TextNode("scoreText");
    scoreText->set_text("");
    scoreText->set_text_color(255.0, 215.0, 0.0, 1);
    NodePath scoreTextNode = window->get_aspect_2d().attach_new_node(scoreText);
    scoreTextNode.set_pos(-1.3, 0.0, 0.90);
    scoreTextNode.set_scale(0.10);

    ballText = new TextNode("ballText");
    ballText->set_text("");
    ballText->set_align(TextNode::A_right);
    ballText->set_text_color(1.0, 0.0, 0.0, 1);
    NodePath ballTextNode = window->get_aspect_2d().attach_new_node(ballText);
    ballTextNode.set_pos(1.3, 0.0, 0.90);
    ballTextNode.set_scale(0.10);

    infoText = new TextNode("infoText");
    infoText->set_text("");
    infoText->set_text_color(0.0, 0.0, 0.0, 1);
    //text2->set_frame_color(0, 0, 1, 1);
    //text2->set_frame_as_margin(0.2, 0.2, 0.1, 0.1);
    infoText->set_card_color(1.0, 1.0, 1.0, 1);
    infoText->set_card_as_margin(0, 0, 0, 0);
    infoText->set_card_decal(true);
    NodePath infoTextNode = window->get_aspect_2d().attach_new_node(infoText);
    infoTextNode.set_pos(-1.3, 0.0, -0.95);
    infoTextNode.set_scale(0.05);
    infoTextHandler((Event*) NULL, (void*) NULL);

    // lights
    initLight(window);
    initShadow(window);

    // task multithread
    //task = new GenericAsyncTask("SceneUpdate", &updateScene, (void *)physics_world);
    task_mgr->add(new GenericAsyncTask("SceneUpdate", &updateScene, (void *)physics_world));
    struct check_collision_data this_check_collision_data;
    this_check_collision_data.physics_world = physics_world;
    this_check_collision_data.window = window;
    task_mgr->add(new GenericAsyncTask("CheckCollision", &checkCollision, (void *)&this_check_collision_data));
    task_mgr->add(new GenericAsyncTask("UpdateText", &updateText, (void *) NULL));

    // set key
    framework.define_key("escape", "callSystemExitFunction", &sysExit,(void *) 0);
    framework.define_key("w", "callKeyHandler", &keyHandler, (void *) &camera);
    framework.define_key("s", "callKeyHandler", &keyHandler, (void *) &camera);
    framework.define_key("a", "callKeyHandler", &keyHandler, (void *) &camera);
    framework.define_key("d", "callKeyHandler", &keyHandler, (void *) &camera);
    framework.define_key("arrow_up", "callKeyHandler", &keyHandler, (void *) &camera);
    framework.define_key("arrow_down", "callKeyHandler", &keyHandler, (void *) &camera);
    framework.define_key("arrow_left", "callKeyHandler", &keyHandler, (void *) &camera);
    framework.define_key("arrow_right", "callKeyHandler", &keyHandler, (void *) &camera);

    // debugger keys
    framework.define_key("f1", "callChangeDebugMode", &debugModeHandler, (void *) bullet_dbg_node);
    framework.define_key("f2", "callChangeDebugMode", &debugModeHandler, (void *) bullet_dbg_node);
    framework.define_key("f3", "callChangeDebugMode", &debugModeHandler, (void *) bullet_dbg_node);
    framework.define_key("f4", "callChangeDebugMode", &debugModeHandler, (void *) bullet_dbg_node);

    // change text status
    framework.define_key("l", "callInfoTextHandler", &infoTextHandler, (void *) NULL);

    PT(AudioManager) audioManager = AudioManager::create_AudioManager();
    PT(AudioSound) mySound =  audioManager->get_sound("../SVDRV_as04/sounds/shot_sound.wav");
    audioManager->set_active(true);

    // data to shoot thread
    shot_data a_shot_data;
    a_shot_data.camera = &camera;
    a_shot_data.physics_world = physics_world;
    a_shot_data.window = window;
    a_shot_data.sound = mySound;
    framework.define_key("space", "shot", &shot,(void *)&a_shot_data);

    framework.main_loop();
    audioManager->shutdown();
    framework.close_framework();
    return 0;
}
