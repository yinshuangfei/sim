add_definitions(
  -D__USE_GNU
)

set(LIB_PREFIX 64)

SET(fsalsim_LIB_SRCS
   main.c
   handle.c
   export.c
   fs.c
   internal.c
)

add_library(fsalsim MODULE ${fsalsim_LIB_SRCS})
add_sanitizers(fsalsim)

target_link_libraries(fsalsim
  ganesha_nfsd
  ${SYSTEM_LIBRARIES}
  ${LTTNG_LIBRARIES}
  ${LDFLAG_DISALLOW_UNDEF}
)

set_target_properties(fsalsim PROPERTIES VERSION 4.2.0 SOVERSION 4)
install(TARGETS fsalsim COMPONENT fsal DESTINATION ${FSAL_DESTINATION} )

