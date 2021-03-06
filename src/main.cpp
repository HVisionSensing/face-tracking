#include <registration.h>
#include <camera_grabber.h>
#include <pcl/console/parse.h>

int main(int argc, char** argv)
{

  std::string database_path,result_path;

  database_path = "PCA.txt";
  result_path = "result";

  double pi =  4 * atan(1);

  double distance_limit = 0.001, angle_limit = pi * 0.25;

  /* Note that if the model is read from a file the scale should be 1.0, otherwise if the model is calculated from a database the scale should be around 0.1 */

  double scale = 1.0;

  double energy_weight = 0.001;

  int device = CV_CAP_OPENNI;

  Registration registrator;

  /* Path to the database of the model */

  pcl::console::parse_argument (argc, argv, "-database", database_path);

  /* Path to the file where the final result is saved */

  pcl::console::parse_argument (argc, argv, "-result", result_path);

  /* The distance threshold for establishing the correspondences */

  pcl::console::parse_argument (argc, argv, "-distance", distance_limit);

  /* The angle threshold between the normals for establishing the correspondences */

  pcl::console::parse_argument (argc, argv, "-angle", angle_limit);

  /* The scale to which the training set has to be brought to */

  pcl::console::parse_argument (argc, argv, "-scale", scale);

  /* The weight that the regularizing part of the Non-Rigid Registration should have */

  pcl::console::parse_argument (argc, argv, "-energy_weight", energy_weight);


  Eigen::Matrix3d transform_matrix = Eigen::Matrix3d::Identity();
  Eigen::Vector3d translation = Eigen::Vector3d::Zero();

  transform_matrix(0,0) = scale;
  transform_matrix(1,1) = scale;
  transform_matrix(2,2) = scale;

  /* For some reason, the cloud returned by the Asus Xtion is always up-side down, so the model is rotated by 180 degrees along the Ox axis */

  transform_matrix = Eigen::AngleAxisd(pi,Eigen::Vector3d::UnitX()) * transform_matrix;


  /* This switch enables the debug mode in order to analyze in the PCLVisualizer the intermiediate steps of the registration */

  bool debug = pcl::console::find_switch (argc, argv, "-debug");

  if( pcl::console::find_switch (argc, argv, "-Asus") )
  {
    device = CV_CAP_OPENNI_ASUS;
  }

  registrator.setDebugMode ( debug );

  /* In this if branch the target cloud is a simple snapshot from the Kinect/Xtion */

  if(pcl::console::find_switch (argc, argv, "--camera"))
  {

    std::string xml_file("haarcascade_frontalface_alt.xml");

    pcl::console::parse_argument (argc, argv, "-xml_file", xml_file);

    registrator.getTargetPointCloudFromCamera(device,xml_file);
    registrator.getDataForModel(database_path, transform_matrix, translation, scale);
    registrator.alignModel();
    registrator.calculateAlternativeRegistrations(50,energy_weight,15,100,angle_limit,distance_limit,debug);

  }

  /* In this if branch the target cloud is read from a pcd file */

  if(pcl::console::find_switch (argc, argv, "--scan"))
  {

    std::string pcd_file("target.pcd");

    pcl::console::parse_argument (argc, argv, "-target", pcd_file);


    float x,y,z;

    pcl::console::parse_argument (argc, argv, "-x", x);
    pcl::console::parse_argument (argc, argv, "-y", y);
    pcl::console::parse_argument (argc, argv, "-z", z);

    pcl::PointXYZ face(x,y,z);

    registrator.getTargetPointCloudFromFile(pcd_file, face);
    registrator.getDataForModel(database_path, transform_matrix, translation, scale);
    registrator.alignModel();
    registrator.calculateAlternativeRegistrations(50,energy_weight,15,100,angle_limit,distance_limit,debug);

  }

  /* In this if branch the target cloud is read by using the KinfuTracker */


  if(pcl::console::find_switch (argc, argv, "--kinfu"))
  {

    registrator.getDataForModel (database_path, transform_matrix, translation, scale);
    registrator.calculateKinfuTrackerRegistrations (device,50,energy_weight,100,angle_limit,distance_limit);
  }


  registrator.writeDataToPCD(result_path);



  return (0);
}
