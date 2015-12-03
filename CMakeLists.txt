PROJECT(ITK-VTK_CLIs)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

## bash command to check if CMakeLists.txt contains an entry for all *.cxx: 
## for i in *cxx ; do if ! `grep -q ${i%.cxx} CMakeLists.txt`; then echo "$i not found"'!'; fi; done


##from http://www.cmake.org/pipermail/cmake/2008-September/023808.html
#i.e. this is bad: SET(CMAKE_BUILD_TYPE "Release")
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their definition
# and dump it in the cache along with proper documentation, otherwise set CMAKE_BUILD_TYPE
# to Debug prior to calling PROJECT()

#${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}.${ITK_VERSION_PATCH} 


IF(DEFINED CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
   SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()


IF(DEFINED CMAKE_INSTALL_PREFIX)
   SET(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE STRING "Choose the install directory.")
ELSE()
   SET(CMAKE_INSTALL_PREFIX /opt/itk-simple/ CACHE STRING "Choose the install directory.")
ENDIF()

##-Werror for clean programming ;-)
# SET(CMAKE_CXX_FLAGS "") #dynamic linking, comment out for static linking
# IF(DEFINED CMAKE_CXX_FLAGS)
#    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror" CACHE STRING "NOT compiled with static libc, libgcc and libstdc++")
# ELSE()
#    SET(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++ -Werror" CACHE STRING "compile with static libc, libgcc and libstdc++")
# ENDIF()


SET(Required_Packages
  ITK
)


FIND_PACKAGE (ITK REQUIRED)
IF ( ITK_FOUND )
      INCLUDE( ${USE_ITK_FILE} )
ENDIF( ITK_FOUND )
FIND_PACKAGE ( VTK )
IF ( VTK_FOUND )
      INCLUDE( ${USE_VTK_FILE} )
ENDIF( VTK_FOUND )

# INCLUDE_DIRECTORIES(
#   )

SET(Libraries
  ${ITK_LIBRARIES}
  ${VTK_LIBRARIES}
)

SET(vtkSplineDrivenImageSlicer "filter/external/vtkSplineDrivenImageSlicer/Filters/")

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ## check if specific modules are enabled
# LIST(FIND ITK_MODULES_ENABLED ITKReview ITKReview_FOUND)


SET(CurrentExe "itk2vti")
ADD_EXECUTABLE(${CurrentExe}  ${CurrentExe})
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)

SET(CurrentExe "straighten")
ADD_EXECUTABLE(${CurrentExe}  ${CurrentExe} "${vtkSplineDrivenImageSlicer}/vtkSplineDrivenImageSlicer.cxx" "${vtkSplineDrivenImageSlicer}/vtkFrenetSerretFrame.cxx")
TARGET_INCLUDE_DIRECTORIES(${CurrentExe} PRIVATE "${vtkSplineDrivenImageSlicer}")
TARGET_LINK_LIBRARIES(${CurrentExe} ${Libraries})
install (TARGETS ${CurrentExe} RUNTIME DESTINATION bin)
