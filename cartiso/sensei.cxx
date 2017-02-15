#include "sensei.h"

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "ConfigurableAnalysis.h"
#include "DataAdaptor.h"

class CartIsoDataAdaptor : public sensei::DataAdaptor
{
public:
  static CartIsoDataAdaptor* New();
  vtkTypeMacro(sensei::DataAdaptor, CartIsoDataAdaptor);
  /// @brief Return the data object with appropriate structure.
  ///
  /// This method will return a data object of the appropriate type. The data
  /// object can be a vtkDataSet subclass or a vtkCompositeDataSet subclass.
  /// If \c structure_only is set to true, then the geometry and topology
  /// information will not be populated. For data adaptors that produce a
  /// vtkCompositeDataSet subclass, passing \c structure_only will still produce
  /// appropriate composite data hierarchy.
  ///
  /// @param structure_only When set to true (default; false) the returned mesh
  /// may not have any geometry or topology information.
  virtual vtkDataObject* GetMesh(bool structure_only=false)
    {
      if (this->NI && this->NJ && this->NK && !this->Mesh)
      {
        this->Mesh = vtkSmartPointer<vtkImageData>::New();
        this->Mesh->SetExtent(this->LocalExtent);
        if (structure_only == false)
        {
          this->AddArray(this->Mesh, vtkDataObject::POINT, this->IsoDataName);
          this->AddArray(this->Mesh, vtkDataObject::POINT, this->PerlinDataName);
        }
      }
      return this->Mesh;
    }

  /// @brief Adds the specified field array to the mesh.
  ///
  /// This method will add the requested array to the mesh, if available. If the
  /// array was already added to the mesh, this will not add it again. The mesh
  /// should not be expected to have geometry or topology information.
  ///
  /// @param association field association; one of
  /// vtkDataObject::FieldAssociations or vtkDataObject::AttributeTypes.
  /// @return true if array was added (or already added), false is request array
  /// is not available.
  virtual bool AddArray(vtkDataObject* mesh, int association, const std::string& arrayname)
    {
      if (association == vtkDataObject::POINT)
      {
        vtkDataSet* ds = vtkDataSet::SafeDownCast(mesh);
        if (this->IsoData && arrayname == this->IsoDataName)
        {
          vtkNew<vtkFloatArray> array;
          array->SetArray(this->IsoData, ds->GetNumberOfPoints(), 1);
          array->SetName(this->IsoDataName.c_str());
          ds->GetPointData()->AddArray(array.GetPointer());
          return true;
        }
        if (this->PerlinData && arrayname == this->PerlinDataName)
        {
          vtkNew<vtkFloatArray> array;
          array->SetArray(this->PerlinData, ds->GetNumberOfPoints(), 1);
          array->SetName(this->PerlinDataName.c_str());
          ds->GetPointData()->AddArray(array.GetPointer());
          return true;
        }
      }
      return false;
    }

  /// @brief Return the number of field arrays available.
  ///
  /// This method will return the number of field arrays available. For data
  /// adaptors producing composite datasets, this is a union of arrays available
  /// on all parts of the composite dataset.
  ///
  /// @param association field association; one of
  /// vtkDataObject::FieldAssociations or vtkDataObject::AttributeTypes.
  /// @return the number of arrays.
  virtual unsigned int GetNumberOfArrays(int association)
    {
      return association == vtkDataObject::POINT ? 2 : 0;
    }

  /// @brief Return the name for a field array.
  ///
  /// This method will return the name for a field array given its index.
  ///
  /// @param association field association; one of
  /// vtkDataObject::FieldAssociations or vtkDataObject::AttributeTypes.
  /// @param index index for the array. Must be less than value returned
  /// GetNumberOfArrays().
  /// @return name of the array.
  virtual std::string GetArrayName(int association, unsigned int index)
    {
      std::string retVal;
      if (association == vtkDataObject::POINT && this->Mesh)
      {
        if (index == 0)
        {
          retVal = this->IsoDataName;
        }
        else if (index == 1)
        {
          retVal = this->PerlinDataName;
        }
      }
      return retVal;
    }

  void SetDataInformation(int tstep, int ni, int nj, int nk, int is, int ie, int js, int je,
                          int ks, int ke, float deltax, float deltay, float deltaz,
                          float *isodata, const char* isodataname, float* perlindata,
                          const char* perlindataname)
    {
      this->ReleaseData(); // just to be safe
      this->SetDataTime((double)tstep); // not any real time to use
      this->SetDataTimeStep(tstep);
      this->NI = ni;
      this->NJ = nj;
      this->NK = nk;
      this->Spacing[0] = deltax;
      this->Spacing[1] = deltay;
      this->Spacing[2] = deltaz;
      this->LocalExtent[0] = is;
      this->LocalExtent[1] = ie;
      this->LocalExtent[2] = js;
      this->LocalExtent[3] = je;
      this->LocalExtent[4] = ks;
      this->LocalExtent[5] = ke;
      this->IsoData = isodata;
      this->IsoDataName = isodataname;
      this->PerlinData = perlindata;
      this->PerlinDataName = perlindataname;
    }

  /// @brief Release data allocated for the current timestep.
  ///
  /// Releases the data allocated for the current timestep. This is expected to
  /// be called after each time iteration.
  virtual void ReleaseData()
    {
      this->Mesh = NULL;
      this->NI = this->NJ = this->NK = 0;
      this->IsoData = NULL;
      this->IsoDataName.clear();
      this->PerlinData = NULL;
      this->PerlinDataName.clear();
    }

protected:
  CartIsoDataAdaptor() : NI(0), NJ(0), NK(0), IsoData(NULL), PerlinData(NULL) {}
  virtual ~CartIsoDataAdaptor() {}

  // global grid sizes
  int NI, NJ, NK;
  // grid spacing
  float Spacing[3];
  // local grid extents
  int LocalExtent[6];

  // Pointer to CartIso's Iso data
  float* IsoData;
  std::string IsoDataName;
  // Pointer to CartIso's Perlin noise data
  float* PerlinData;
  std::string PerlinDataName;

  vtkSmartPointer<vtkImageData> Mesh;

private:
  CartIsoDataAdaptor(const CartIsoDataAdaptor&); // Not implemented.
  void operator=(const CartIsoDataAdaptor&); // Not implemented.
};

struct DataAdaptors
{
  vtkSmartPointer<CartIsoDataAdaptor> DataAdaptor;
  vtkSmartPointer<sensei::ConfigurableAnalysis> AnalysisAdaptor;
};

vtkStandardNewMacro(CartIsoDataAdaptor);

extern "C" void* senseiInitialize(const char* xmlFileName)
{
  DataAdaptors* adaptors = new DataAdaptors;
  adaptors->DataAdaptor = vtkSmartPointer<CartIsoDataAdaptor>::New();
  adaptors->AnalysisAdaptor = vtkSmartPointer<sensei::ConfigurableAnalysis>::New();
  std::string fname = xmlFileName;
  adaptors->AnalysisAdaptor->Initialize(MPI_COMM_WORLD, fname);

  return static_cast<void*>(adaptors);
}

extern "C" void senseiFinalize(void* cp)
{
  if(cp)
  {
    DataAdaptors* adaptors = (DataAdaptors*) cp;
    delete adaptors;
    adaptors = NULL;
  }
}

extern "C" void senseiOutput(void* cp, int tstep,
                             int ni, int nj, int nk, int is, int ie, int js, int je,
                             int ks, int ke, float deltax, float deltay, float deltaz,
                             float *isodata, const char* isodataname, float* perlindata,
                             const char* perlindataname)
{
  if(!cp) return;

  DataAdaptors* adaptors = (DataAdaptors*) cp;

  adaptors->DataAdaptor->SetDataInformation(
    tstep, ni, nj, nk, is, ie, js, je, ks, ke, deltax, deltay, deltaz,
    isodata, isodataname, perlindata, perlindataname);
  adaptors->AnalysisAdaptor->Execute(adaptors->DataAdaptor);
}
