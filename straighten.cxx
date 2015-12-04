////program for vtkSplineDrivenImageSlicer
//01: based on template.cxx and StraightenedReformattedVolume.cxx from J. Velut (http://hdl.handle.net/10380/3318)


#include <itkCommand.h>
#include <itkImageFileReader.h>
#include <itkImageToVTKImageFilter.h>
#include <itkVTKImageToImageFilter.h>
#include <itkImageFileWriter.h>

#include <vtkSmartPointer.h>
#include <vtkCallbackCommand.h>
#include <vtkXMLPolyDataReader.h>
#include "vtkSplineDrivenImageSlicer.h"
#include <vtkImageAppend.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkXMLPolyDataWriter.h>

#include <set>


template<typename ReaderImageType, typename WriterImageType>
void FilterEventHandlerITK(itk::Object *caller, const itk::EventObject &event, void*){

    const itk::ProcessObject* filter = static_cast<const itk::ProcessObject*>(caller);

    if(itk::ProgressEvent().CheckEvent(&event))
        fprintf(stderr, "\r%s progress: %5.1f%%", filter->GetNameOfClass(), 100.0 * filter->GetProgress());//stderr is flushed directly
    else if(itk::StartEvent().CheckEvent(&event)){
        if(strstr(filter->GetNameOfClass(), "ImageFileReader"))
            std::cerr << "Reading: " << (dynamic_cast<itk::ImageFileReader<ReaderImageType> *>(caller))->GetFileName() << std::endl;//cast only works if reader was instanciated for ReaderImageType!
        else if(strstr(filter->GetNameOfClass(), "ImageFileWriter"))
            std::cerr << "Writing: " << (dynamic_cast<itk::ImageFileWriter<WriterImageType> *>(caller))->GetFileName() << std::endl;//cast only works if writer was instanciated for WriterImageType!
        }
    // else if(itk::IterationEvent().CheckEvent(&event))
    //     std::cerr << " Iteration: " << (dynamic_cast<itk::SliceBySliceImageFilter<ReaderImageType, WriterImageType> *>(caller))->GetSliceIndex() << std::endl;
    else if(itk::EndEvent().CheckEvent(&event))
        std::cerr << std::endl;
    }

void FilterEventHandlerVTK(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData){

    vtkAlgorithm *filter= static_cast<vtkAlgorithm*>(caller);

    switch(eventId){
    case vtkCommand::ProgressEvent:
        fprintf(stderr, "\r%s progress: %5.1f%%", filter->GetClassName(), 100.0 * filter->GetProgress());//stderr is flushed directly
        break;
    case vtkCommand::EndEvent:
        std::cerr << std::endl << std::flush;
        break;
        }
    }


template<typename InputComponentType, typename InputPixelType, size_t Dimension>
int DoIt(int argc, char *argv[]){

    typedef InputPixelType  OutputPixelType;

    typedef itk::Image<InputPixelType, Dimension>  InputImageType;
    typedef itk::Image<OutputPixelType, Dimension>  OutputImageType;

    itk::CStyleCommand::Pointer eventCallbackITK;
    eventCallbackITK = itk::CStyleCommand::New();
    eventCallbackITK->SetCallback(FilterEventHandlerITK<InputImageType, OutputImageType>);

    vtkSmartPointer<vtkCallbackCommand> eventCallbackVTK = vtkSmartPointer<vtkCallbackCommand>::New();
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);

    char* outPrefix= argv[9];
    std::stringstream sss;

    typedef itk::ImageFileReader<InputImageType> ReaderType;
    typename ReaderType::Pointer reader = ReaderType::New();

    reader->SetFileName(argv[1]);
    reader->ReleaseDataFlagOn();
    reader->AddObserver(itk::AnyEvent(), eventCallbackITK);
    try{
        reader->Update();
        }
    catch(itk::ExceptionObject &ex){
        std::cerr << ex << std::endl;
        return EXIT_FAILURE;
        }

    const typename InputImageType::Pointer& input= reader->GetOutput();

    typedef itk::ImageToVTKImageFilter<InputImageType> ITK2VTKType;
    typename ITK2VTKType::Pointer itk2vtk= ITK2VTKType::New();
    itk2vtk->SetInput(input);
    itk2vtk->ReleaseDataFlagOn();
    try{
        itk2vtk->Update();
        }
    catch(itk::ExceptionObject &ex){
        std::cerr << ex << std::endl;
        return EXIT_FAILURE;
        }


    vtkSmartPointer<vtkXMLPolyDataReader> pathReader= vtkSmartPointer<vtkXMLPolyDataReader>::New();
    pathReader->SetFileName(argv[2]);
    pathReader->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    pathReader->Update();

    double spacing= itk2vtk->GetOutput()->GetSpacing()[0];//expecting isotropic spacing!
    vtkSmartPointer<vtkSplineDrivenImageSlicer> reslicer= vtkSmartPointer<vtkSplineDrivenImageSlicer>::New();
    reslicer->SetInputData(itk2vtk->GetOutput());
    reslicer->SetPathConnection(pathReader->GetOutputPort());
    reslicer->SetSliceExtent(atoi(argv[5]), atoi(argv[6]));
    reslicer->SetSliceSpacing(spacing, spacing);
    reslicer->SetSliceThickness(atof(argv[7]));
    reslicer->SetInterpolationMode(atoi(argv[8]));
    reslicer->UseSliceSpacingOff();//drastically reduces polyDataOutput, sufficient for texture mapping
    
    unsigned int every= 1000;
    std::set<vtkIdType> sliceSet;
  
    if(argc > 12)
	for(vtkIdType i= 11; i < argc; i++)
	    sliceSet.insert(atoi(argv[i])); 
    else
	every= atoi(argv[11]);


    std::set<vtkIdType>::iterator it;
    std::cerr << "sliceSet: ";
    for (it=sliceSet.begin(); it!=sliceSet.end(); ++it)
	std::cerr << ' ' << *it;
    std::cerr << std::endl;

    vtkSmartPointer<vtkImageAppend> append= vtkSmartPointer<vtkImageAppend>::New();


    vtkIdType nbPoints = pathReader->GetOutput()->GetNumberOfPoints();
    for(vtkIdType ptId = 0; ptId < nbPoints; ptId++){
	const bool is_in= sliceSet.find(ptId) != sliceSet.end(); // http://stackoverflow.com/a/1701083
	if(((argc == 12) && !(ptId % every)) || is_in )
	    reslicer->SetProbeInput(atoi(argv[10]));
	else
	    reslicer->ProbeInputOff();

        reslicer->SetOffsetPoint(ptId);
        reslicer->Update();

        vtkSmartPointer<vtkImageData> tempSlice;
        tempSlice= vtkSmartPointer<vtkImageData>::New();
        tempSlice->DeepCopy(reslicer->GetOutput(0));

        append->AddInputData(tempSlice);

	if(((argc == 12) && !(ptId % every)) || is_in ){
	    vtkSmartPointer<vtkXMLPolyDataWriter> writerPD= vtkSmartPointer<vtkXMLPolyDataWriter>::New();
	    sss.str(""); sss << outPrefix << "_" << std::setfill('0') << std::setw(4) << ptId << ".vtp";
	    writerPD->SetFileName(sss.str().c_str());
	    writerPD->SetInputConnection(reslicer->GetOutputPort(1));
	    writerPD->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()
	    if(atoi(argv[4]))
		writerPD->SetCompressorTypeToZLib();//default
	    else
		writerPD->SetCompressorTypeToNone();
	    writerPD->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
	    writerPD->Write();
	    }

        fprintf(stderr, "\r%s progress: %5.1f%%", "Reslicing", 100.0 * ptId/nbPoints);
        }
    std::cerr << " done." << std::endl;

    append->SetAppendAxis(2);
    append->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    append->Update();


    typedef itk::VTKImageToImageFilter<InputImageType> VTK2ITKType;
    typename VTK2ITKType::Pointer vtk2itk= VTK2ITKType::New();
    vtk2itk->SetInput(append->GetOutput());
    vtk2itk->ReleaseDataFlagOn();
    try{
        vtk2itk->Update();
        }
    catch(itk::ExceptionObject &ex){
        std::cerr << ex << std::endl;
        return EXIT_FAILURE;
        }

    typedef itk::ImageFileWriter<OutputImageType>  WriterType;
    typename WriterType::Pointer writer = WriterType::New();

    writer->SetFileName(argv[3]);
    writer->SetInput(vtk2itk->GetOutput());
    writer->SetUseCompression(atoi(argv[4]));
    writer->AddObserver(itk::AnyEvent(), eventCallbackITK);
    try{
        writer->Update();
        }
    catch(itk::ExceptionObject &ex){
        std::cerr << ex << std::endl;
        return EXIT_FAILURE;
        }

    return EXIT_SUCCESS;

    }


template<typename InputComponentType, typename InputPixelType>
int dispatch_D(size_t dimensionType, int argc, char *argv[]){
    int res= EXIT_FAILURE;
    switch (dimensionType){
    case 3:
        res= DoIt<InputComponentType, InputPixelType, 3>(argc, argv);
        break;
    default:
        std::cerr << "Error: Images of dimension " << dimensionType << " are not handled!" << std::endl;
        break;
        }//switch
    return res;
    }

template<typename InputComponentType>
int dispatch_pT(itk::ImageIOBase::IOPixelType pixelType, size_t dimensionType, int argc, char *argv[]){
    int res= EXIT_FAILURE;
    //http://www.itk.org/Doxygen45/html/classitk_1_1ImageIOBase.html#abd189f096c2a1b3ea559bc3e4849f658
    //http://www.itk.org/Doxygen45/html/itkImageIOBase_8h_source.html#l00099
    //IOPixelType:: UNKNOWNPIXELTYPE, SCALAR, RGB, RGBA, OFFSET, VECTOR, POINT, COVARIANTVECTOR, SYMMETRICSECONDRANKTENSOR, DIFFUSIONTENSOR3D, COMPLEX, FIXEDARRAY, MATRIX

    switch (pixelType){
    case itk::ImageIOBase::SCALAR:{
        typedef InputComponentType InputPixelType;
        res= dispatch_D<InputComponentType, InputPixelType>(dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::UNKNOWNPIXELTYPE:
    default:
        std::cerr << std::endl << "Error: Pixel type not handled!" << std::endl;
        break;
        }//switch
    return res;
    }

int dispatch_cT(itk::ImageIOBase::IOComponentType componentType, itk::ImageIOBase::IOPixelType pixelType, size_t dimensionType, int argc, char *argv[]){
    int res= EXIT_FAILURE;

    //http://www.itk.org/Doxygen45/html/classitk_1_1ImageIOBase.html#a8dc783055a0af6f0a5a26cb080feb178
    //http://www.itk.org/Doxygen45/html/itkImageIOBase_8h_source.html#l00107
    //IOComponentType: UNKNOWNCOMPONENTTYPE, UCHAR, CHAR, USHORT, SHORT, UINT, INT, ULONG, LONG, FLOAT, DOUBLE

    switch (componentType){
    case itk::ImageIOBase::UCHAR:{        // uint8_t
        typedef unsigned char InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::CHAR:{         // int8_t
        typedef char InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::USHORT:{       // uint16_t
        typedef unsigned short InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::SHORT:{        // int16_t
        typedef short InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::UINT:{         // uint32_t
        typedef unsigned int InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::INT:{          // int32_t
        typedef int InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::ULONG:{        // uint64_t
        typedef unsigned long InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::LONG:{         // int64_t
        typedef long InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::FLOAT:{        // float32
        typedef float InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::DOUBLE:{       // float64
        typedef double InputComponentType;
        res= dispatch_pT<InputComponentType>(pixelType, dimensionType, argc, argv);
        } break;
    case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
    default:
        std::cerr << "unknown component type" << std::endl;
        break;
        }//switch
    return res;
    }


////from http://itk-users.7.n7.nabble.com/Pad-image-with-0-but-keep-its-type-what-ever-it-is-td27442.html
//namespace itk{
  // Description:
  // Get the PixelType and ComponentType from fileName

void GetImageType (std::string fileName,
    itk::ImageIOBase::IOPixelType &pixelType,
    itk::ImageIOBase::IOComponentType &componentType,
    size_t &dimensionType
    ){
    typedef itk::Image<char, 1> ImageType; //template initialization parameters need to be given but can be arbitrary here
    itk::ImageFileReader<ImageType>::Pointer imageReader= itk::ImageFileReader<ImageType>::New();
    imageReader->SetFileName(fileName.c_str());
    imageReader->UpdateOutputInformation();

    pixelType = imageReader->GetImageIO()->GetPixelType();
    componentType = imageReader->GetImageIO()->GetComponentType();
    dimensionType= imageReader->GetImageIO()->GetNumberOfDimensions();

    std::cerr << std::endl << "dimensions: " << dimensionType << std::endl;
    std::cerr << "component type: " << imageReader->GetImageIO()->GetComponentTypeAsString(componentType) << std::endl;
    std::cerr << "component size: " << imageReader->GetImageIO()->GetComponentSize() << std::endl;
    std::cerr << "pixel type (string): " << imageReader->GetImageIO()->GetPixelTypeAsString(imageReader->GetImageIO()->GetPixelType()) << std::endl;
    std::cerr << "pixel type: " << pixelType << std::endl << std::endl;

    }



int main(int argc, char *argv[]){
    if ( argc < 9 ){
        std::cerr << "Missing Parameters: "
                  << argv[0]
                  << " Input_Image"
                  << " Input_Path"
                  << " Output_Image"
                  << " compress"
                  << " x-extent y-extent"
                  << " avg_z-spacing"
                  << " interpolation-mode"
                  << " [outpuPD] [probe-scalars] [slice-interv | [slice1] [slice2] ...]"
                  << std::endl;

        return EXIT_FAILURE;
        }

    itk::ImageIOBase::IOPixelType pixelType;
    typename itk::ImageIOBase::IOComponentType componentType;
    size_t dimensionType;


    try {
        GetImageType(argv[1], pixelType, componentType, dimensionType);
        }//try
    catch( itk::ExceptionObject &excep){
        std::cerr << argv[0] << ": exception caught !" << std::endl;
        std::cerr << excep << std::endl;
        return EXIT_FAILURE;
        }

    return dispatch_cT(componentType, pixelType, dimensionType, argc, argv);
    }






