#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <ignition/math/Pose3.hh>
#include <ignition/math/Vector3.hh>
#include <string_view>
#include <gazebo/physics/Collision.hh>

namespace gazebo
{
  class WorldUuvPlugin : public WorldPlugin
  {

  enum states{
    unconnectable_unlocked, connectable_unlocked, connectable_locked
  };

  // private: states state = unconnectable_unlcoked;

  private: physics::WorldPtr world;

  private: physics::ModelPtr buffer;

  private: physics::ModelPtr plugModel;

  private: physics::LinkPtr plugLink;

  private: physics::ModelPtr boxModel;

  private: physics::LinkPtr boxLink;

  private: physics::ModelPtr socketModel;

  private: physics::LinkPtr socketLink;

  private: ignition::math::Pose3d socket_pose;

  private: ignition::math::Pose3d plug_pose;

  private: physics::JointPtr prismaticJoint;

  public: ignition::math::Vector3d grabAxis;

  public: ignition::math::Vector3d grabbedForce;

  public: physics::CollisionPtr collisionPtr;

  // private: Common::Time connectedTime;



  private: bool joined = false;

  private: gazebo::event::ConnectionPtr updateConnection;


  public: WorldUuvPlugin() : WorldPlugin(){}

  public: void Load(physics::WorldPtr _world, sdf::ElementPtr _sdf){
      this->world = _world;
      this->socketModel = this->world->ModelByName("electrical_socket");
      this->plugModel = this->world->ModelByName("electrical_plug");
      this->boxModel = this->world->ModelByName("box");
      
      this->socketLink = this->socketModel->GetLink("socket");
      this->plugLink = this->plugModel->GetLink("plug");
      this->boxLink = this->boxModel->GetLink("box");

      this->updateConnection = gazebo::event::Events::ConnectWorldUpdateBegin(
          std::bind(&WorldUuvPlugin::Update, this));
    }

  private: bool checkRollAlignment(){
      ignition::math::Vector3<double> socketRotation = socketModel->RelativePose().Rot().Euler();
      ignition::math::Vector3<double> plugRotation = plugModel->RelativePose().Rot().Euler();
      return abs(plugRotation[0] - socketRotation[0]) < 0.1;
    }

  private: bool checkPitchAlignment(){
      ignition::math::Vector3<double> socketRotation = socketModel->RelativePose().Rot().Euler();
      ignition::math::Vector3<double> plugRotation = plugModel->RelativePose().Rot().Euler();
      return abs(plugRotation[1] - socketRotation[1]) < 0.1;
    }

  private:bool checkYawAlignment(){
      ignition::math::Vector3<double> socketRotation = socketModel->RelativePose().Rot().Euler();
      ignition::math::Vector3<double> plugRotation = plugModel->RelativePose().Rot().Euler();
      return abs(plugRotation[2] - socketRotation[2]) < 0.1;
    }

  private: bool checkRotationalAlignment()
    {
      if (this->checkYawAlignment() && this->checkPitchAlignment() && this->checkRollAlignment())
      {
        // printf("Aligned, ready for insertion  \n");
        return true;
      }
      else
      {
        // printf("Disaligned, not ready for mating  \n");
        return false;
      }
    }

  private: bool checkVerticalAlignment()
    {
      socket_pose = socketModel->RelativePose();
      ignition::math::Vector3<double> socketPositon = socket_pose.Pos();
      // printf("%s  \n", typeid(socketPositon).name());

      plug_pose = plugModel->RelativePose();
      ignition::math::Vector3<double> plugPosition = plug_pose.Pos();

      bool onSameVerticalLevel = abs(plugPosition[2] - socketPositon[2]) < 0.1;
      if (onSameVerticalLevel)
      {
        // printf("z=%.2f  \n", plugPosition[2]);
        // printf("Share same vertical level  \n");
        return true;
      }
      return false;
    }

  private: bool isAlligned()
    {
      if(checkVerticalAlignment() && checkRotationalAlignment()){
        return true;
      } else {
        return false;
      }
    }

  private: bool checkProximity()
    {
      socket_pose = socketModel->WorldPose();
      // socket_pose = socketLink->RelativePose();
      ignition::math::Vector3<double> socketPositon = socket_pose.Pos();
      // printf("%s  \n", typeid(socketPositon).name());

      plug_pose = plugModel->RelativePose();
      ignition::math::Vector3<double> plugPosition = plug_pose.Pos();
      // printf("%f %f %f Within Proximity  \n", plugPosition[0], plugPosition[1], plugPosition[2]);
      float xdiff_squared = pow(abs(plugPosition[0] - socketPositon[0]),2);
      float ydiff_squared = pow(abs(plugPosition[1] - socketPositon[1]),2);
      float zdiff_squared = pow(abs(plugPosition[2] - socketPositon[2]),2);

      bool withinProximity = pow(xdiff_squared+ydiff_squared+zdiff_squared,0.5) < 1;
      if (withinProximity)
      {
        // printf("%f Within Proximity  \n", plugPosition[0]);
        return true;
      }else {

        // printf("not within Proximity  \n");
      }
      return false;
    }

  private: bool isReadyForInsertion()
    {
      if (this->checkProximity() && this->isAlligned()){
        return true;
        // printf("Insert \n");

      } else{

        // printf("Cant insert \n");
        return false;
      }
    }

  public: void freezeJoint(physics::JointPtr prismaticJoint){
    gazebo::math::Angle currentPosition = prismaticJoint->GetAngle(0);
    joint->SetHighStop(0, currentPosition);
    joint->SetLowStop(0, currentPosition);
  }
  
  public: void Update()
    {
      // connect the socket and the plug after 5 seconds
      if (this->world->SimTime() > 2.0 && joined == false)
      {
        this->joined = true;
        // prismaticJoint = world->Physics()->CreateJoint("prismatic");
        // TODO!!
        // ok! there are two ways to make joints
        // first way: _world->GetPhysicsEngine()->CreateJoint
        // http://osrf-distributions.s3.amazonaws.com/gazebo/api/9.0.0/classgazebo_1_1physics_1_1PhysicsEngine.html#aa4bdca668480d14312458f78fab7687d

        // second way: 
        // https://osrf-distributions.s3.amazonaws.com/gazebo/api/dev/classgazebo_1_1physics_1_1Model.html#ad7e10b77c7c7f9dc09b3fdc41d00846e
        printf("%d \n", plugModel->GetChildCount());

        this->prismaticJoint = plugModel->CreateJoint(
          "random_joint_name",
          "prismatic",
          socketLink,
          plugLink);
        
        if (this->prismaticJoint){
        printf("7amra \n");

        }
        
        printf("%d \n", plugModel->GetChildCount());
        plugModel->SetSelfCollide(true);
        if(plugModel->GetSelfCollide()){
          printf("well, this should be working\n");
        }else{

          printf("not working\n");
        }
        gzmsg << this->prismaticJoint->GetScopedName(true) << "\n";
        gzmsg << this->prismaticJoint->GetChild()->GetName() << "\n";
        gzmsg << this->prismaticJoint->GetParent()->GetName() << "\n";

        // prismaticJoint = world->Physics()->CreateJoint(
        // );
      //   prismaticJoint->SetName(this->socketLink->GetName() + std::string("_") +
      //                 this->plugLink->GetName() + std::string("_joint"));
        gzmsg << this->prismaticJoint->GetScopedName(true) << "\n";
        gzmsg << this->prismaticJoint->GetScopedName(false) << "\n";
      //   prismaticJoint->SetModel(this->plugModel);
      //   gzmsg << this->prismaticJoint->GetScopedName(false) << "\n";
      //   // prismaticJoint = world->Physics()->CreateJoint("prismatic", this->socket);
      //   prismaticJoint->Attach(this->socketLink, this->plugLink);
        prismaticJoint->Load(this->socketLink, this->plugLink, 
          ignition::math::Pose3<double>(ignition::math::Vector3<double>(1, 0, 0), 
          ignition::math::Quaternion<double>(0, 0, 0, 0)));
      //     // ignition::math::Quaternion<double>(0, 0.3428978, 0, 0.9393727)));
        prismaticJoint->SetUpperLimit(0, 0.3);
        // prismaticJoint->SetLowerLimit(0, -0.1);
        prismaticJoint->Init();
        prismaticJoint->SetAxis(0, ignition::math::Vector3<double>(1, 0, 0));
        prismaticJoint->SetAnchor(0, ignition::math::Vector3<double>(1, 0, 0));
      }

      if (joined){
        plugModel->SetSelfCollide(true);
        collisionPtr = socketLink->GetCollision("Link");
        if (collisionPtr){
          printf("well seems to exist! \n");
          // printf("%s  \n", typeid(collisionPtr).name());
          gzmsg << collisionPtr->GetName() << "\n";
          world->SetPaused(true);
          

        } else {

          // printf("nop \n");
        }



          // printf("%s   \n", collisionPtr->GetName());

        // GetChildCollision
      //   // prismaticJoint->SetAxis(0, ignition::math::Vector3<double>(1, 0, 0));
        // prismaticJoint->SetVelocity(0, -0.1);
        // prismaticJoint->SetParam("friction", 0, .05);
        // plugLink->AddForce(ignition::math::Vector3<double>(-100, 0, 0));
        // prismaticJoint->SetForce(0, -70);
        grabAxis = prismaticJoint->LocalAxis(0);
        // printf("%f %f %f   \n", grabAxis[0], grabAxis[1], grabAxis[2]);
        // socketLink->SetCollideMode("all");
        // plugLink->SetCollideMode("all");


        // grabbedForce = socketLink->RelativeForce();
        // printf("%.2f %.2f %.2f   || ", abs(grabbedForce[0]), abs(grabbedForce[1]), abs(grabbedForce[2]));

        // GetName
        // CollisionPtr 
        // GetCollision
        grabbedForce = boxLink->RelativeForce();
        if (abs(grabbedForce[0])>10){
        // if (abs(grabbedForce[2])>1){
          printf("%.2f %.2f %.2f   \n", abs(grabbedForce[0]), abs(grabbedForce[1]), abs(grabbedForce[2]));

        }
      //   // printf("%.2f %.2f %.2f   \n", grabbedForce[0], grabbedForce[1], grabbedForce[2]);
      //   // grabbedForce = 
        
      }

      // this->checkVerticalAlignment();
      // this->checkRotationalAlignment();
      // this->checkProximity();
      // printf("%s  \n",FormattedString);
      // if (this->isReadyForInsertion()){

      // } else{
      //   connectedTime = 0;
      // }
    }
  };
  GZ_REGISTER_WORLD_PLUGIN(WorldUuvPlugin)
} // namespace gazebo