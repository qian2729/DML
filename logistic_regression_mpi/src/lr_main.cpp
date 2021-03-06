#include <string>
#include "mpi.h"
#include "load_data.h"
#include "owlqn.h"
#include "ftrl.h"
#include "predict.h"
#include <glog/logging.h>
//#include "gtest/gtest.h"

int main(int argc,char* argv[]){  
    int rank, nproc;
    int namelen = 1024;
    char processor_name[namelen];
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);
    MPI_Get_processor_name(processor_name,&namelen);
    // Initialize Google's logging library.
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = "./log";
    LOG(INFO) << "my process rank: "<< rank <<", totoal process num: "<< nproc <<std::endl;
    std::cout<<"my host = "<<processor_name<<" my rank = "<<rank<<std::endl;

    char train_data_path[1024];
    const char *train_data_file = argv[2];
    snprintf(train_data_path, 1024, "%s-%05d", train_data_file, rank);
    char test_data_path[1024];
    const char *test_data_file = argv[3];
    snprintf(test_data_path, 1024, "%s-%05d", test_data_file, rank);

    Load_Data train_data(train_data_path); 
    train_data.load_data_batch(nproc, rank);
    MPI_Barrier(MPI_COMM_WORLD);

    std::vector<float> model;

    if (strcmp(argv[1], "owlqn") == 0){
        OWLQN owlqn(&train_data, nproc, rank);
        owlqn.owlqn();
        for(int j = 0; j < train_data.glo_fea_dim; j++){
            std::cout<<"w["<< j << "]: "<<owlqn.glo_w[j]<<std::endl;
            model.push_back(owlqn.glo_w[j]);
        }
    }
    else if(strcmp(argv[1], "ftrl") == 0){
        FTRL ftrl(&train_data, nproc, rank);
        ftrl.ftrl();
        std::cout<<"rank main "<<rank<<std::endl;
        for(int j = 0; j < train_data.glo_fea_dim; j++){
	        //if(ftrl.loc_w[j] != 0)std::cout<<"w["<< j << "]: "<<ftrl.loc_w[j]<<std::endl;
	        model.push_back(ftrl.loc_w[j]);
        }
    }
    std::cout<<"rank "<<rank<<" finish train! "<<processor_name<<std::endl;
    MPI_Barrier(MPI_COMM_WORLD); 

    Load_Data test_data(test_data_path);
    test_data.load_data_batch(nproc, rank);
    Predict predict(&test_data, nproc, rank);
    predict.run(model);

    MPI::Finalize();
    return 0;
}
