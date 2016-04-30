#include <sstream>
#include <string>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <uvm/uvm_extern.h> // uvmexp struct

#include "error.h" 
#include "conversions.h"
#include "memory.h"

void mem_status( MemoryStatus & status )
{
  // get vm memory stats
  static int vm_totalmem[] = { CTL_VM, VM_UVMEXP2 };
  struct uvmexp_sysctl mem;
  size_t size = sizeof( mem );
  if( sysctl( vm_totalmem, 2, &mem, &size, NULL, 0 ) < 0 )
  {
    error( "memory: error getting vm memory stats" );
  }

  int64_t total_mem = ( mem.npages << mem.pageshift );
  int64_t used_mem =
    ( mem.active + mem.wired - mem.filepages ) << mem.pageshift;

  // add 1 to used which gets lost somewhere along conversions
  status.used_mem = convert_unit(static_cast< float >( used_mem ), MEGABYTES );
  status.total_mem = convert_unit(static_cast< float >( total_mem ), MEGABYTES );
}
