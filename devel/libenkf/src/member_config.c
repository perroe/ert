#include <stdlib.h>
#include <util.h>
#include <member_config.h>
#include <time_t_vector.h>
#include <stdbool.h>
#include <path_fmt.h>
#include <subst_list.h>
#include <sched_file.h>
#include <ecl_config.h>
#include <enkf_fs.h>
#include <enkf_types.h>
#include <ensemble_config.h>
#include <time.h>


/**
   This struct contains information which is private to this
   member. It is initialized at object boot time, and (typically) not
   changed during the simulation. [In principle it could change during
   the simulation, but the current API does not support that.]
*/ 


struct member_config_struct {
  int  		          iens;                /* The ensemble member number of this member. */
  char                  * casename;            /* The name of this case - will mosttly be NULL. */
  keep_runpath_type       keep_runpath;        /* Should the run-path directory be left around (for this member)*/
  char 		        * eclbase;             /* The ECLBASE string used for simulations of this member. */
  const sched_file_type * sched_file;          /* The schedule file - a shared pointer NOT owned bt this object. */ 
  int                     last_restart_nr;
  time_t_vector_type    * report_time;         /* This vector contains the (per member) report_step -> simulation_time mapping. */
};




/******************************************************************/
/** Implementation of the member_config struct. All of this implementation
    is private - however some of it is exported through the enkf_state object,
    and it should be perfectly safe to export more of it.
*/



const char * member_config_set_eclbase(member_config_type * member_config , const ecl_config_type * ecl_config , const subst_list_type * subst_list) {
  util_safe_free( member_config->eclbase );
  {
    char * tmp = path_fmt_alloc_path(ecl_config_get_eclbase_fmt(ecl_config), false , member_config->iens);
    member_config->eclbase = subst_list_alloc_filtered_string( subst_list , tmp );
    free( tmp );
  }
  return member_config->eclbase;
}


int member_config_get_iens( const member_config_type * member_config ) {
  return member_config->iens;
}

int member_config_get_last_restart_nr( const member_config_type * member_config) {
  return member_config->last_restart_nr;
}
				   

void member_config_free(member_config_type * member_config) {
  util_safe_free(member_config->eclbase);
  util_safe_free(member_config->casename );
  time_t_vector_free( member_config->report_time );
  free(member_config);
}



static void member_config_set_keep_runpath(member_config_type * member_config , keep_runpath_type keep_runpath) {
  member_config->keep_runpath   = keep_runpath;
}




const sched_file_type * member_config_get_sched_file( const member_config_type * member_config) {
  return member_config->sched_file;
}

keep_runpath_type member_config_get_keep_runpath(const member_config_type * member_config) {
  return member_config->keep_runpath;
}


void member_config_iset_sim_time( member_config_type * member_config , int report_step , time_t sim_time ) {
  time_t_vector_iset( member_config->report_time , report_step , sim_time );
}


/**
   This function will return the default value (i.e. -1) if the input
   report_step is invalid. The calling scope must check for this.

   
   Historical note:
   ----------------

   Time in the ert codebase is in terms of ECLIPS report
   steps. Originally these were translated to 'true time' via the
   schedule file. In newer implementations this is done as follows:

    1. The member_config object contains a time_t vector which
       translates from report_step to time_t.

    2. The elements in this time_t vector are set with the
       member_config_iset_sim_time() function when loading summary
       data.

    3. The time_t vector of the member_config object is stored as an
       enkf_fs case_member file.

   To support older cases, where the time_t vector has not been
   stored, this function will fall back to the schedule file if it
   does not have the report step which is asked for. In the latter
   situation the time_t vector is updated. If the fs pointer is !=
   NULL the updated vector is written to disk, i.e. the ensemble is
   'upgraded'.

   This is introduced at svn =~ 2300 = 23/11/2009, when this has been
   in place for sufficiently long time, the fallback to schedule
   should be removed.
*/


time_t member_config_iget_sim_time( member_config_type * member_config , int report_step , enkf_fs_type * fs) {
  time_t sim_time = time_t_vector_safe_iget( member_config->report_time , report_step );
  
  if (sim_time == -1) {
    /* Fall back to check with the schedule file. */
    sim_time = sched_file_get_sim_time( member_config->sched_file , report_step );
    member_config_iset_sim_time( member_config , report_step , sim_time );
    if (fs != NULL)
      member_config_fwrite_sim_time( member_config , fs );
  }
  return sim_time;
}


/**
   Will return -1 if the data are not available. 
*/
double member_config_iget_sim_days( member_config_type * member_config , int report_step, enkf_fs_type * fs) {
  time_t start_time = member_config_iget_sim_time( member_config , 0           , fs );
  time_t sim_time   = member_config_iget_sim_time( member_config , report_step , fs );

  if (sim_time >= start_time)
    return 1.0 * (sim_time - start_time) / (3600 * 24);
  else
    return -1;
  
}



bool member_config_has_report_step( const member_config_type * member_config , int report_step) {
  if (time_t_vector_size( member_config->report_time ) > report_step)
    return false;
  else {
    time_t def_time = time_t_vector_get_default( member_config->report_time );
    if ( time_t_vector_iget( member_config->report_time , report_step ) != def_time )
      return true;
    else
      return false;
  }
}


int member_config_get_sim_length( const member_config_type * member_config ) {
  return time_t_vector_size( member_config->report_time );
}


static void member_config_fread_sim_time( member_config_type * member_config , enkf_fs_type * enkf_fs) {
  FILE * stream = enkf_fs_open_excase_member_file( enkf_fs , "sim_time" , member_config->iens );
  if (stream != NULL) {
    time_t_vector_fread( member_config->report_time , stream );
    fclose( stream );
  }
}

void member_config_fwrite_sim_time( const member_config_type * member_config , enkf_fs_type * enkf_fs ) {
  FILE * stream = enkf_fs_open_case_member_file( enkf_fs , "sim_time" , member_config->iens , "w");
  time_t_vector_fwrite( member_config->report_time , stream );
  fclose( stream );
}


const char * member_config_get_eclbase( const member_config_type * member_config ) {
  return member_config->eclbase;
}


const char * member_config_get_casename( const member_config_type * member_config ) {
  return member_config->casename;
}


member_config_type * member_config_alloc(int iens , 
                                         const char * casename , 
                                         keep_runpath_type            keep_runpath , 
                                         const ecl_config_type      * ecl_config , 
                                         const ensemble_config_type * ensemble_config,
                                         enkf_fs_type               * fs) { 
						
  member_config_type * member_config = util_malloc(sizeof * member_config , __func__);
  member_config->casename            = util_alloc_string_copy( casename );
  member_config->iens                = iens; /* Can only be changed in the allocater. */
  member_config->eclbase  	     = NULL;
  member_config_set_keep_runpath(member_config , keep_runpath);
  member_config->sched_file         = ecl_config_get_sched_file( ecl_config );
  member_config->last_restart_nr  = sched_file_get_num_restart_files( member_config->sched_file ) - 1; /* Fuck me +/- 1 */
  member_config->report_time = time_t_vector_alloc( 0 , -1 );
  member_config_fread_sim_time( member_config , fs );
  return member_config;
}