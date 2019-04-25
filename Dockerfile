################################################################################
# base system
################################################################################
FROM ubuntu:18.04 as system


################################################################################
# builder
################################################################################
FROM system as builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    ca-certificates `# essential for git over https` \
    cmake \
    build-essential

### VTK
RUN git clone -b v7.1.1 --depth 1 https://gitlab.kitware.com/vtk/vtk.git

RUN mkdir -p VTK_build && \
    cd VTK_build && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/vtk/ \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DBUILD_SHARED_LIBS=ON \
	  -DBUILD_TESTING=OFF \
	  -DVTK_Group_Qt=OFF \
	  -DVTK_Group_Rendering=OFF \
	  -DVTK_Group_StandAlone=ON \
	  -DVTK_RENDERING_BACKEND=None \
	  -DModule_vtkInteractionStyle=ON \
	  ../vtk && \
    make -j"$(nproc)" && \
    make -j"$(nproc)" install


### ITK
RUN git clone https://github.com/InsightSoftwareConsortium/ITK.git `# GH for PRs ` && \
    cd ITK && \
    git fetch origin pull/765/head:vtk-rendering-optional `# get PR ` && \
    git checkout vtk-rendering-optional

RUN mkdir -p ITK_build && \
    cd ITK_build && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/itk/ \
	  -DCMAKE_MODULE_PATH=/opt/vtk/lib/cmake \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DBUILD_SHARED_LIBS=ON \
	  -DBUILD_TESTING=OFF \
	  -DModule_ITKVtkGlue=ON \
	  ../ITK && \
    make -j"$(nproc)" && \
    make -j"$(nproc)" install


### ITK-VTK-CLIs
COPY . /code/

RUN mkdir -p /build/ && \
    cd /build/ && \
    cmake \
    	  -DCMAKE_INSTALL_PREFIX=/opt/ITK-VTK-CLIs/ \
	  -DCMAKE_PREFIX_PATH=/opt/vtk/lib/cmake/ \
	  -DCMAKE_BUILD_TYPE=Release \
	  /code/ && \
    make -j"$(nproc)" && \
    make -j"$(nproc)" install


################################################################################
# install
################################################################################
FROM system as install

COPY --from=builder /opt/vtk/ /opt/vtk/
COPY --from=builder /opt/itk/ /opt/itk/
COPY --from=builder /opt/ITK-VTK-CLIs/ /opt/ITK-VTK-CLIs/

ENV LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:/opt/vtk/lib/"
ENV LD_LIBRARY_PATH "${LD_LIBRARY_PATH}:/opt/itk/lib/"

ENV PATH "/opt/ITK-VTK-CLIs/bin/:${PATH}"

WORKDIR /data
