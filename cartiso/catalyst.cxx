#include "catalyst.h"

#include "vtkCPDataDescription.h"
#include "vtkCPInputDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCPPythonScriptPipeline.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

extern "C" void* catalystInitialize(const char* scriptName)
{
  vtkCPProcessor* catalystProcessor = vtkCPProcessor::New();
  catalystProcessor->Initialize();
  vtkCPPythonScriptPipeline* catalystScript =
    vtkCPPythonScriptPipeline::New();
  catalystScript->Initialize(scriptName);
  catalystProcessor->AddPipeline(catalystScript);
  catalystScript->Delete();
  return static_cast<void*>(catalystProcessor);
}

extern "C" void catalystFinalize(void* cp)
{
  if(cp) {
      vtkCPProcessor* catalystProcessor = (vtkCPProcessor*) cp;
      catalystProcessor->Finalize();
      catalystProcessor->Delete();
  }
}

extern "C" void catalystOutput(void* cp, int tstep,
                               int ni, int nj, int nk, int is, int ie, int js, int je,
                               int ks, int ke, float deltax, float deltay, float deltaz,
                               float *data, const char* dataName, float* xdata, const char* xdataName,
                               uint64_t ntris, float *points, float *norms,
                               float *trixdata, const char *trixdataName)
{
  if(!cp) return;

  vtkCPProcessor* catalystProcessor = (vtkCPProcessor*) cp;
  vtkNew<vtkCPDataDescription> dataDescription;
  dataDescription->SetTimeData((float)tstep, tstep);
  dataDescription->AddInput("cart");
  dataDescription->AddInput("iso");
  if(catalystProcessor->RequestDataDescription(dataDescription.GetPointer())) {
      // The Cartesian grid...
      vtkCPInputDataDescription* cartInput =
        dataDescription->GetInputDescriptionByName("cart");
      // Check if the Cartesian grid is needed
      if(cartInput->GetGenerateMesh()) {
          vtkNew<vtkImageData> imageData;
          imageData->SetSpacing(deltax, deltay, deltaz);
          imageData->SetExtent(is, ie, js, je, ks, ke);

          // Check if the data array is needed
          if(cartInput->IsFieldNeeded(dataName)) {
              vtkNew<vtkFloatArray> vtkdata;
              vtkdata->SetArray(data, imageData->GetNumberOfPoints(), 1);
              vtkdata->SetName(dataName);
              imageData->GetPointData()->AddArray(vtkdata.GetPointer());
          }
          // Check if the xdata array is needed
          if(cartInput->IsFieldNeeded(xdataName)) {
              vtkNew<vtkFloatArray> vtkxdata;
              vtkxdata->SetArray(xdata, imageData->GetNumberOfPoints(), 1);
              vtkxdata->SetName(xdataName);
              imageData->GetPointData()->AddArray(vtkxdata.GetPointer());
          }

          cartInput->SetGrid(imageData.GetPointer());
          cartInput->SetWholeExtent(0, ni-1, 0, nj-1, 0, nk-1);
      }

      // The iso grid...
      vtkCPInputDataDescription* isoInput =
        dataDescription->GetInputDescriptionByName("iso");
      // Check if the isosurface is needed
      if(isoInput->GetGenerateMesh()) {
          vtkNew<vtkPolyData> polyData;
          vtkNew<vtkFloatArray> vtkpointsarray;
          vtkpointsarray->SetNumberOfComponents(3);
          vtkpointsarray->SetArray(points, static_cast<vtkIdType>(ntris*3), 1);
          vtkNew<vtkPoints> vtkpoints;
          vtkpoints->SetData(vtkpointsarray.GetPointer());
          polyData->SetPoints(vtkpoints.GetPointer());
          polyData->Allocate(ntris);
          vtkIdType pointIds[3];
          for(uint64_t i=0;i<ntris;i++) {
              pointIds[0] = static_cast<vtkIdType>(i*3);
              pointIds[1] = static_cast<vtkIdType>(i*3+1);
              pointIds[2] = static_cast<vtkIdType>(i*3+2);
              polyData->InsertNextCell(VTK_TRIANGLE, 3, pointIds);
          }

          // Check if the trixdata array is needed
          if(isoInput->IsFieldNeeded(trixdataName)) {
              vtkNew<vtkFloatArray> vtktrixdata;
              vtktrixdata->SetArray(trixdata, polyData->GetNumberOfPoints(), 1);
              vtktrixdata->SetName(trixdataName);
              polyData->GetPointData()->AddArray(vtktrixdata.GetPointer());
          }

          isoInput->SetGrid(polyData.GetPointer());
      }

      catalystProcessor->CoProcess(dataDescription.GetPointer());
  }
}
