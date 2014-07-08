#ifndef REGISTRATION_H
#define REGISTRATION_H

#include "position_model.h"
#include <pcl/io/pcd_io.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/features/normal_3d.h>
#include <pcl/visualization/pcl_visualizer.h>



class Registration
{
  public:

    Registration();
    ~Registration();

    void
    readDataFromOBJFiles (std::string source_points_path, std::string target_points_path);

    void
    readOBJFile (std::string file_path, pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud, Eigen::Matrix3d transform_matrix, Eigen::Vector3d translation, bool advance = true, int number_vertices = 4);


    void
    readDataFromOBJFileAndPCDScan(std::string source_points_path, std::string target_points_path, Eigen::Matrix3d transform_matrix, Eigen::Vector3d translation);

    void
    getDataFromModel (std::string database_path, std::string target_points_path, Eigen::MatrixX3d transformation_matrix, Eigen::Vector3d translation);

    void
    calculateRigidTransformation (int number_of_iterations, double angle_limit, double distance_limit);

    void
    calculateNonRigidTransformation();

    void
    calculateAlternativeTransformations(int number_of_total_iterations, int number_of_rigid_iterations, double angle_limit, double distance_limit);

    void
    applyRigidTransformation ();

    void
    writeDataToPCD (std::string file_path);

    void
    setKdTree (pcl::PointCloud<pcl::PointXYZ>::Ptr target_point_cloud_ptr);

    void
    convertPointCloudToEigen();

    void
    convertEigenToPointCLoud();



  private:


    void
    mouseEventOccurred (const pcl::visualization::MouseEvent &event, void* viewer_void);


    Eigen::Matrix4d homogeneus_matrix_;


    pcl::search::KdTree<pcl::PointXYZRGBNormal> kdtree_;





    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr source_point_normal_cloud_ptr_;
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr target_point_normal_cloud_ptr_;
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr rigid_transformed_points_ptr_;
    pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr iteration_source_point_normal_cloud_ptr_;

    pcl::visualization::PCLVisualizer::Ptr visualizer_ptr_;


    PositionModel* position_model_;

    std::vector <  pcl::Vertices > model_meshes_;

    Eigen::VectorXd eigen_source_points_;
    Eigen::VectorXd eigen_target_points_;
    Eigen::MatrixXd eigenvectors_matrix_;



};

#endif // REGISTRATION_H
