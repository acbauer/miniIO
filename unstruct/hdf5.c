#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <hdf5.h>

#include <pdirs.h>

void writehdf5(char *name, MPI_Comm comm, int tstep, uint64_t npoints, uint64_t nptstask,
               float *xpts, float *ypts, float *zpts, uint64_t nelems3, uint64_t *conns3,
               uint64_t nelems2, uint64_t *conns2, char *varname, float *data);

static const int fnstrmax = 4095;

void writehdf5(char *name, MPI_Comm comm, int tstep, uint64_t npoints, uint64_t nptstask, 
               float *xpts, float *ypts, float *zpts, uint64_t nelems3, uint64_t *conns3,
               uint64_t nelems2, uint64_t *conns2, char *varname, float *data)
{
    char dirname[fnstrmax+1];
    char fname[fnstrmax+1];
    int rank, nprocs;
    int timedigits = 4;
    MPI_Info info = MPI_INFO_NULL;

    hid_t file_id;
    hid_t plist_id;
    hid_t group_id;
    hid_t memspace;
    hid_t filespace;
    
    hid_t did[3];
    hsize_t start[1], count[1];
    hsize_t dims[1];
    herr_t err;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* Make dir for all output and subdir for timestep */
    snprintf(dirname, fnstrmax, "%s.przm", name);
    mkdir1task(dirname, comm);
    snprintf(dirname, fnstrmax, "%s.przm/t%0*d.d", name, timedigits, tstep);
    mkdir1task(dirname, comm);

    /* Set up MPI info */
    MPI_Info_create(&info);
    MPI_Info_set(info, "striping_factor", "1");    

    chkdir1task(dirname, comm);

    snprintf(fname, fnstrmax, "%s/r.h5", dirname);

    /* Set up file access property list with parallel I/O access */
    if( (plist_id = H5Pcreate(H5P_FILE_ACCESS)) < 0) {
      printf("writehdf5 error: Could not create property list \n");
      MPI_Abort(comm, 1);
    }

    if(H5Pset_fapl_mpio(plist_id, MPI_COMM_WORLD, info) < 0) {
      printf("writehdf5 error: Could not create property list \n");
      MPI_Abort(comm, 1);
    }
    
    if( (file_id = H5Fcreate(fname, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id)) < 0) {
      fprintf(stderr, "writehdf5 error: could not open %s\n", fname);
      MPI_Abort(comm, 1);
    }
    
    if(H5Pclose(plist_id) < 0) {
      printf("writehdf5 error: Could not close property list \n");
      MPI_Abort(comm, 1);
    }

    
    /* Optional grid points */
    if(xpts && ypts && zpts) {
      /* Create the dataspace for the dataset. */
      dims[0] = (hsize_t)npoints;
      filespace = H5Screate_simple(1, dims, NULL);

      /* Create Grid Group */
      group_id = H5Gcreate(file_id, "grid points", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

      /* Create the dataset with default properties and close filespace. */
      did[0] = H5Dcreate(group_id, "x", H5T_NATIVE_FLOAT, filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      did[1] = H5Dcreate(group_id, "y", H5T_NATIVE_FLOAT, filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      did[2] = H5Dcreate(group_id, "z", H5T_NATIVE_FLOAT, filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose(filespace);

      /* 
       * Each process defines dataset in memory and writes it to the hyperslab
       * in the file.
       */
      start[0] =(hsize_t)(nptstask*rank);
      count[0] =(hsize_t)nptstask;
      
      memspace = H5Screate_simple(1, count, NULL);
      
      /* Select hyperslab in the file.*/
      filespace = H5Dget_space(did[0]);
      H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, NULL, count, NULL );

      /* Create property list for collective dataset write. */
      plist_id = H5Pcreate(H5P_DATASET_XFER);
      H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);

      err = H5Dwrite (did[0], H5T_NATIVE_FLOAT, memspace, filespace, plist_id, xpts);
      err = H5Dwrite (did[1], H5T_NATIVE_FLOAT, memspace, filespace, plist_id, ypts);
      err = H5Dwrite (did[2], H5T_NATIVE_FLOAT, memspace, filespace, plist_id, zpts);
      

      err = H5Dclose(did[0]);
      err = H5Dclose(did[1]);
      err = H5Dclose(did[2]);
      
      err = H5Sclose(filespace);
      err = H5Sclose(memspace);
      err = H5Gclose(group_id);
    
    }
    if(H5Pclose(plist_id) < 0)
      printf("writehdf5 error: Could not close property list \n");

    uint64_t nelems_in[2];
    uint64_t nelems_out[2];
    
    nelems_in[0] = nelems3 ;
    nelems_in[1] = nelems2 ;

    MPI_Allreduce( nelems_in, nelems_out, 2, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD );

    //MSB is it possible that some processors have 0?

    /* Optional grid connections, writes a 64-bit 0 if no connections */
    if(conns3 && nelems3) {

    /* Create the dataspace for the dataset. */
      dims[0] = (hsize_t)nelems_out[0]*6;
      filespace = H5Screate_simple(1, dims, NULL);

      /* Create the dataset with default properties and close filespace. */
      did[0] = H5Dcreate(file_id, "conns3", H5T_NATIVE_ULLONG, filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose(filespace);

      /* 
       * Each process defines dataset in memory and writes it to the hyperslab
       * in the file.
       */
      start[0] =(hsize_t)(nelems3*6*rank);
      count[0] =(hsize_t)nelems3*6;
      
      memspace = H5Screate_simple(1, count, NULL);
      
      /* Select hyperslab in the file.*/
      filespace = H5Dget_space(did[0]);
      H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, NULL, count, NULL );

      /* Create property list for collective dataset write. */
      plist_id = H5Pcreate(H5P_DATASET_XFER);
      H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
      err = H5Dwrite (did[0], H5T_NATIVE_ULLONG, memspace, filespace, plist_id, conns3);
      

      err = H5Dclose(did[0]);
      
      err = H5Sclose(filespace);
      err = H5Sclose(memspace);

    }

/*     Optional 2D surface triangle connections, writes a 64-bit 0 if none */
    if(conns2 && nelems2) {

      /* Create the dataspace for the dataset. */
      dims[0] = (hsize_t)nelems_out[1]*3;
      filespace = H5Screate_simple(1, dims, NULL);

      /* Create the dataset with default properties and close filespace. */
      did[0] = H5Dcreate(file_id, "conns2", H5T_NATIVE_ULLONG, filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose(filespace);

      /* 
       * Each process defines dataset in memory and writes it to the hyperslab
       * in the file.
       */
      start[0] =(hsize_t)(nelems2*3*rank);
      count[0] =(hsize_t)nelems2*3;
      
      memspace = H5Screate_simple(1, count, NULL);
      
      /* Select hyperslab in the file.*/
      filespace = H5Dget_space(did[0]);
      H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, NULL, count, NULL );

      /* Create property list for collective dataset write. */
      plist_id = H5Pcreate(H5P_DATASET_XFER);
      H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
      err = H5Dwrite (did[0], H5T_NATIVE_ULLONG, memspace, filespace, plist_id, conns3);
      

      err = H5Dclose(did[0]);
      
      err = H5Sclose(filespace);
      err = H5Sclose(memspace);

    } 

    /* Optional variable data, starting with number of variables */
    if(data && varname) {
      /* Create the dataspace for the dataset. */
      dims[0] = (hsize_t)npoints;
      filespace = H5Screate_simple(1, dims, NULL);

      /* Create the dataset with default properties and close filespace. */
      did[0] = H5Dcreate(file_id, "vars", H5T_NATIVE_FLOAT, filespace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      H5Sclose(filespace);

      /* 
       * Each process defines dataset in memory and writes it to the hyperslab
       * in the file.
       */
      start[0] =(hsize_t)(nptstask*rank);
      count[0] =(hsize_t)nptstask;
      
      memspace = H5Screate_simple(1, count, NULL);
      
      /* Select hyperslab in the file.*/
      filespace = H5Dget_space(did[0]);
      H5Sselect_hyperslab(filespace, H5S_SELECT_SET, start, NULL, count, NULL );

      /* Create property list for collective dataset write. */
      plist_id = H5Pcreate(H5P_DATASET_XFER);
      H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_COLLECTIVE);
      err = H5Dwrite (did[0], H5T_NATIVE_FLOAT, memspace, filespace, plist_id, data);
      

      if(H5Dclose(did[0]) ){
	printf("writehdf5 error: Could not close HDF5 data space \n");
	MPI_Abort(comm, 1);
      }
      
      if(H5Sclose(filespace)) {
	printf("writehdf5 error: Could not close HDF5 file space \n");
	MPI_Abort(comm, 1);
      }
      if(H5Sclose(memspace)) {
	printf("writehdf5 error: Could not close HDF5 memory space \n");
	MPI_Abort(comm, 1);
      }


 /*    dims[0] = 4; */
/*     attr[0] = hasgrid; */
/*     attr[1] = hasconn; */
/*     attr[2] = hasconn2; */
/*     attr[3] = nvars; */

/*     space_id = H5Screate_simple (1, dims, NULL); */
/*     attr_id = H5Acreate2(file_id, "hasgrid, hasconn, hasconn2, nvars", */
/*     			 H5T_NATIVE_UINT, space_id, H5P_DEFAULT, H5P_DEFAULT ); */
/*     err = H5Awrite(attr_id, H5T_NATIVE_UINT, attr); */
/*     err = H5Aclose(attr_id); */
/*     err = H5Sclose(space_id); */


    if(H5Fclose(file_id) != 0)
      printf("writehdf5 error: Could not close HDF5 file \n");

}

