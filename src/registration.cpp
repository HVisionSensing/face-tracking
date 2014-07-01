#include <registration.h>

Registration::Registration()
{
  homogeneus_matrix_ = Eigen::Matrix4d::Identity();
  position_model_ = NULL;

  vis_source_point_cloud_ptr_.reset(new pcl::PointCloud<pcl::PointXYZRGB>);
  vis_scan_point_cloud_ptr_.reset(new pcl::PointCloud<pcl::PointXYZRGB>);
  vis_model_point_cloud_ptr_.reset(new pcl::PointCloud<pcl::PointXYZRGB>);

  source_point_normal_cloud_ptr_.reset(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
  target_point_normal_cloud_ptr_.reset(new pcl::PointCloud<pcl::PointXYZRGBNormal>);
  rigid_transformed_points_ptr_.reset(new pcl::PointCloud<pcl::PointXYZRGBNormal>);

}

Registration::~Registration()
{
  if( position_model_ )
  {
    PCL_INFO ("Model deleted\n");
    delete position_model_;
  }

}

void
Registration::readDataFromOBJFiles(std::string source_points_path, std::string target_points_path)
{

  readOBJFile(source_points_path,source_point_normal_cloud_ptr_, Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero());

  readOBJFile(target_points_path, target_point_normal_cloud_ptr_, Eigen::Matrix3d::Identity(), Eigen::Vector3d::Zero());


  kdtree_.setInputCloud(target_point_normal_cloud_ptr_);



}

void
Registration::readOBJFile(std::string file_path, pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr cloud, Eigen::Matrix3d transform_matrix, Eigen::Vector3d translation, bool advance)
{

  std::ifstream instream(file_path.c_str());
  std::string line,value;
  int k;
  size_t index;

  pcl::PointXYZRGBNormal point;

  if(instream.fail())
  {
    PCL_ERROR("Could not open file %s\n", file_path.c_str());
    exit(1);
  }



  while(1)
  {
    std::getline(instream,line);

    std::stringstream ss(line);
    std::string s;

    ss >> s;
    if(s.compare("v") != 0)
      break;

    ss >> point.x;
    ss >> point.y;
    ss >> point.z;


    cloud->push_back(point);


  }

  if (advance)
  {

    std::vector < std::vector < Eigen::Vector3f > > normal_vector(cloud->points.size());
    std::vector < std::vector < float > > surface_vector(cloud->points.size());

    while(line[0] != 'f')
    {
      std::getline(instream,line);
    }

    while(line[0] == 'f' && !instream.eof())
    {
      int mesh[3];
      Eigen::Vector3f normal;

      for(k = 0; k < 3; ++k)
      {
        index = line.find(' ');
        line = line.substr(index+1);
        index = line.find(' ');
        value = line.substr(0,index);
        mesh[k] = (boost::lexical_cast<int>(value));
      }

      Eigen::Vector3f V1,V2;

      V1 = cloud->points[mesh[1]].getVector3fMap() - cloud->points[mesh[0]].getVector3fMap();
      V2 = cloud->points[mesh[2]].getVector3fMap() - cloud->points[mesh[0]].getVector3fMap();


      normal = V1.cross(V2);

      float area = 0.5 * normal.norm();

      for(k = 0; k < 3; ++k)
      {
        normal_vector[mesh[k]].push_back(normal);
        surface_vector[mesh[k]].push_back(area);
      }



      std::getline(instream,line);

    }

    for( k = 0; k < cloud->points.size(); ++k)
    {
      float ratio;
      Eigen::Vector3d normal_result;

      ratio = surface_vector[k][0] + surface_vector[k][1] + surface_vector[k][2];

      normal_result = ( ( surface_vector[k][0] * normal_vector[k][0] ) + ( ( surface_vector[k][1] * normal_vector[k][1] ) + ( surface_vector[k][2] * normal_vector[k][2] ) ) ) / ratio;

      cloud->points[k].normal_x = normal_result[0];
      cloud->points[k].normal_y = normal_result[1];
      cloud->points[k].normal_z = normal_result[2];
    }

  }

  Eigen::Matrix4d homogeneus_transform;

  homogeneus_transform.block(0, 0, 3, 3) = transform_matrix;
  homogeneus_transform.block(0, 3, 3, 1) = translation;
  homogeneus_transform.row(3) << 0, 0, 0, 1;

  pcl::PointCloud<pcl::PointXYZRGBNormal> result_point_cloud;

  pcl::transformPointCloudWithNormals(*cloud,result_point_cloud,homogeneus_transform);

  *cloud = result_point_cloud;



  instream.close();


}

void
Registration::readDataFromOBJFileAndPCDScan(std::string source_points_path, std::string target_points_path, Eigen::Matrix3d transform_matrix, Eigen::Vector3d translation)
{

  int i;

  pcl::PointCloud<pcl::PointXYZ>::Ptr target_point_cloud_ptr(new pcl::PointCloud<pcl::PointXYZ>);

  readOBJFile(source_points_path,source_point_normal_cloud_ptr_, transform_matrix, translation);


  if (pcl::io::loadPCDFile<pcl::PointXYZ> (target_points_path, *target_point_cloud_ptr) == -1) //* load the file
  {
    PCL_ERROR("Could not open file %s\n", target_points_path.c_str());
    exit(1);
  }
/*
  pcl::StatisticalOutlierRemoval<pcl::PointXYZ> outliers_filter;
  outliers_filter.setInputCloud(scan_cloud);
  outliers_filter.setMeanK(50);
  outliers_filter.setStddevMulThresh(10);
  outliers_filter.filter(*target_point_cloud_ptr);
*/
  //pcl::io::savePCDFileASCII("filtered_cloud.pcd", *target_point_cloud_ptr);

  setKdTree(target_point_cloud_ptr);



}

void
Registration::getDataFromModel(std::string database_path, std::string output_path, Eigen::MatrixX3d rotation, Eigen::Vector3d translation)
{
  int i;

  pcl::PointXYZ point;

  std::vector < Eigen::Vector3d > target_points;


  pcl::PointCloud<pcl::PointXYZ>::Ptr target_point_cloud_ptr(new pcl::PointCloud<pcl::PointXYZ>);




  if(!position_model_)
  {
    PCL_INFO ("Created Model\n");
    position_model_ = new PositionModel;
  }

  position_model_->readDataFromFolders(database_path,150,4);
  position_model_->calculateMeanFace();

  position_model_->calculateEigenVectors();
  position_model_->printEigenValues();
  position_model_->calculateRandomWeights(50,output_path);
  position_model_->calculateModel();
  position_model_->writeModel(output_path);

  position_model_->writeMeanFaceAndRotatedMeanFace(rotation, translation, output_path + "_source.obj", output_path +"_transformed.obj",source_point_normal_cloud_ptr_,target_point_normal_cloud_ptr_);


  target_point_cloud_ptr->width = target_points.size();
  target_point_cloud_ptr->height = 1;

  for( i = 0; i < target_points.size(); ++i)
  {
    point.x = target_points[i][0];
    point.y = target_points[i][1];
    point.z = target_points[i][2];

    target_point_cloud_ptr->points.push_back(point);
  }

  setKdTree(target_point_cloud_ptr);





}

void
Registration::calculateRigidTransformation(int number_of_iterations)
{

  PCL_INFO("In calculate method\n");
  int i,j;


  pcl::visualization::PCLVisualizer viewer("3D Viewer");
  viewer.setBackgroundColor(0, 0, 0);
  viewer.initCameraParameters();

  uint32_t rgb;
  uint8_t value(255);

  rgb = ((uint32_t)value) << 16;


  for( i = 0; i < target_point_normal_cloud_ptr_->points.size(); ++i)
  {
      source_point_normal_cloud_ptr_->points[i].rgb = *reinterpret_cast<float*>(&rgb);
  }


  rgb = ((uint32_t)value) << 8;


  for( i = 0; i < target_point_normal_cloud_ptr_->points.size(); ++i)
  {
      target_point_normal_cloud_ptr_->points[i].rgb = *reinterpret_cast<float*>(&rgb);
  }


  Eigen::MatrixXd JJ, J_transpose, J(source_point_normal_cloud_ptr_->points.size(), 6);
  Eigen::VectorXd y(source_point_normal_cloud_ptr_->points.size());
  Eigen::VectorXd solutions;

  Eigen::Matrix3d current_iteration_rotation = Eigen::Matrix3d::Identity();
  Eigen::Vector3d current_iteration_translation;

  pcl::PointCloud<pcl::PointXYZRGBNormal> current_iteration_source_points = *source_point_normal_cloud_ptr_;
  pcl::PointCloud<pcl::PointXYZRGBNormal>::Ptr current_iteration_source_points_ptr (&current_iteration_source_points);


  for(j = 0; j < number_of_iterations; ++j)
  {

    PCL_INFO("Iteration %d\n",j);

    pcl::Correspondences iteration_correspondences;


    for(i = 0; i < current_iteration_source_points.size(); ++i)
    {

      pcl::PointXYZRGBNormal search_point;


      search_point = current_iteration_source_points.at(i);

      std::vector < int > point_index(1);
      std::vector < float > point_distance(1);

      kdtree_.nearestKSearch(search_point,1,point_index,point_distance);



      Eigen::Vector3d cross_product, normal,eigen_point;

      normal[0] = target_point_normal_cloud_ptr_->points[point_index[0]].normal_x;
      normal[1] = target_point_normal_cloud_ptr_->points[point_index[0]].normal_y;
      normal[2] = target_point_normal_cloud_ptr_->points[point_index[0]].normal_z;

      Eigen::Vector3d aux_vector;

      aux_vector = (current_iteration_source_points[i].getVector3fMap()).cast<double>();

      cross_product = aux_vector.cross(normal);

      eigen_point[0] = target_point_normal_cloud_ptr_->points[point_index[0]].x - search_point.x;
      eigen_point[1] = target_point_normal_cloud_ptr_->points[point_index[0]].y - search_point.y;
      eigen_point[2] = target_point_normal_cloud_ptr_->points[point_index[0]].z - search_point.z;


      pcl::Correspondence correspondence(i,point_index[0],point_distance[0]);

      iteration_correspondences.push_back(correspondence);

      y[i] = eigen_point.dot(normal);

      J(i,0) = cross_product[0];
      J(i,1) = cross_product[1];
      J(i,2) = cross_product[2];
      J(i,3) = normal[0];
      J(i,4) = normal[1];
      J(i,5) = normal[2];


    }



    viewer.addPointCloud <pcl::PointXYZRGBNormal> (current_iteration_source_points_ptr,"source");
    viewer.addPointCloud <pcl::PointXYZRGBNormal> (target_point_normal_cloud_ptr_,"scan");
    viewer.addCorrespondences <pcl::PointXYZRGBNormal> (current_iteration_source_points_ptr,target_point_normal_cloud_ptr_,iteration_correspondences);

    viewer.spin();



    viewer.removeAllShapes();
    viewer.removeAllPointClouds();
    viewer.removeCorrespondences();


    J_transpose = J.transpose();
    JJ = J_transpose * J;

    Eigen::MatrixXd right_side = J_transpose * y;



    solutions = JJ.colPivHouseholderQr().solve(right_side);

    PCL_INFO("Done with linear Solvers\n");


    current_iteration_rotation = Eigen::AngleAxisd(solutions[0],Eigen::Vector3d::UnitX()) * Eigen::AngleAxisd(solutions[1],Eigen::Vector3d::UnitY()) * Eigen::AngleAxisd(solutions[2],Eigen::Vector3d::UnitZ()) ;


    for(i = 0; i < 3; ++i)
    {
      current_iteration_translation(i) = solutions(i+3);
    }



    Eigen::Matrix4d current_homogeneus_matrix,resulting_homogeneus_matrix;

    current_homogeneus_matrix.block(0, 0, 3, 3) = current_iteration_rotation;
    current_homogeneus_matrix.block(0, 3, 3, 1) = current_iteration_translation;
    current_homogeneus_matrix.row(3) << 0, 0, 0, 1;

    pcl::PointCloud<pcl::PointXYZRGBNormal> result;

    pcl::transformPointCloudWithNormals(current_iteration_source_points,result,current_homogeneus_matrix);

    current_iteration_source_points = result;


    resulting_homogeneus_matrix = current_homogeneus_matrix * homogeneus_matrix_;

/*
    double difference = ( resulting_homogeneus_matrix - homogeneus_matrix_ ).norm();

    if(difference < 0.1)
        break;
*/
    homogeneus_matrix_ = resulting_homogeneus_matrix;

    //homogeneus_matrices_vector_.push_back(current_homogeneus_matrix);

    //iteration_correspondences_vector_.push_back(iteration_correspondences);




  }

}


void
Registration::applyRigidTransformation()
{

  pcl::transformPointCloudWithNormals (*source_point_normal_cloud_ptr_,*rigid_transformed_points_ptr_,homogeneus_matrix_);

  uint32_t rgb;
  uint8_t value(255);

  rgb = ((uint32_t)value);

  for( int i = 0; i < rigid_transformed_points_ptr_->points.size(); ++i)
  {
      rigid_transformed_points_ptr_->points[i].rgb = *reinterpret_cast<float*>(&rgb);
  }



}

void
Registration::writeDataToPCD(std::string file_path)
{
  pcl::PointCloud < pcl::PointXYZRGBNormal > initial_cloud, rigid_cloud, output_cloud, target_cloud;
  pcl::PointXYZRGB point;
  int i;
  uint32_t rgb;
  uint8_t value(255);
/*
  pcl::copyPointCloud(*source_point_normal_cloud_ptr_,initial_cloud);


  rgb = ((uint32_t)value) << 16;

  for( i = 0; i < initial_cloud.points.size(); ++i)
  {
      initial_cloud.points[i].rgb = *reinterpret_cast<float*>(&rgb);
  }

  pcl::copyPointCloud(*rigid_transformed_points_ptr_,rigid_cloud);

  rgb = ((uint32_t)value);

  for( i = 0; i < rigid_cloud.points.size(); ++i)
  {
      rigid_cloud.points[i].rgb = *reinterpret_cast<float*>(&rgb);
  }


  pcl::copyPointCloud(*target_point_normal_cloud_ptr_,target_cloud);


  rgb = ((uint32_t)value) << 8;

  for( i = 0; i < target_cloud.points.size(); ++i)
  {
      target_cloud.points[i].rgb = *reinterpret_cast<float*>(&rgb);
  }
*/
  output_cloud = *source_point_normal_cloud_ptr_ + (*rigid_transformed_points_ptr_ + *target_point_normal_cloud_ptr_);

  pcl::PCDWriter pcd_writer;

  pcd_writer.writeBinary < pcl::PointXYZRGBNormal > (file_path + ".pcd", output_cloud);



}

void
Registration::setKdTree(pcl::PointCloud<pcl::PointXYZ>::Ptr target_point_cloud_ptr)
{

  pcl::PointCloud<pcl::Normal>::Ptr target_normal_cloud_ptr(new pcl::PointCloud<pcl::Normal>);
  pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> normal_estimator;
  pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);


  normal_estimator.setInputCloud(target_point_cloud_ptr);
  tree->setInputCloud(target_point_cloud_ptr);
  normal_estimator.setSearchMethod(tree);
  normal_estimator.setKSearch(10);
  //normal_estimator.setRadiusSearch(0.10);
  normal_estimator.compute(*target_normal_cloud_ptr);

  pcl::concatenateFields (*target_point_cloud_ptr, *target_normal_cloud_ptr, *target_point_normal_cloud_ptr_);

  kdtree_.setInputCloud(target_point_normal_cloud_ptr_);

}

/*
void
Registration::visualizeCorrespondences()
{
  int i;
  pcl::visualization::PCLVisualizer viewer("3D Viewer");
  viewer.setBackgroundColor(0, 0, 0);

  viewer.initCameraParameters();

  for(i = 0; i < iteration_correspondences_vector_.size(); ++i)
  {

    viewer.addPointCloud(vis_source_point_cloud_ptr_,"source");
    viewer.addPointCloud(vis_scan_point_cloud_ptr_,"scan");
    viewer.addCorrespondences <pcl::PointXYZRGB> (vis_source_point_cloud_ptr_,vis_scan_point_cloud_ptr_,iteration_correspondences_vector_[i]);

    viewer.spin();

    pcl::PointCloud<pcl::PointXYZRGB> result;

    //pcl::transformPointCloud (*vis_source_point_cloud_ptr_,result,homogeneus_matrices_vector_[i]);

    *vis_source_point_cloud_ptr_ = result;



    viewer.removeAllShapes();
    viewer.removeAllPointClouds();
    viewer.removeCorrespondences();

  }

}
*/
