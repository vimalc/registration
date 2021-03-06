#ifndef METRICVALUEWRITER_HPP_
#define METRICVALUEWRITER_HPP_

#include "MetricValueWriterBase.hpp"

class MetricValueWriter : public MetricValueWriterBase
{
public:
  typedef MetricValueWriter        Self;
  typedef MetricValueWriterBase    Superclass;
  typedef itk::SmartPointer<Self>  Pointer;
  
  itkNewMacro( Self );
  
  virtual string dirPath()
  {
    return m_outputRootDir + transformType() + "/";
  }
  
};

#endif
