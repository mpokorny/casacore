//# CompressComplex.cc: Virtual column engine to scale a table Complex array
//# Copyright (C) 2001,2002
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$

//# Includes
#include <aips/Tables/CompressComplex.h>
#include <aips/Tables/Table.h>
#include <aips/Tables/ArrayColumn.h>
#include <aips/Tables/ColumnDesc.h>
#include <aips/Tables/TableRecord.h>
#include <aips/Tables/DataManError.h>
#include <aips/Arrays/ArrayIter.h>
#include <aips/Arrays/Slicer.h>
#include <aips/Containers/Record.h>
#include <aips/Mathematics/Math.h>
#include <aips/Utilities/String.h>



CompressComplex::CompressComplex (const String& sourceColumnName,
				  const String& targetColumnName,
				  Float scale, Float offset)
: BaseMappedArrayEngine<Complex,Int> (sourceColumnName, targetColumnName),
  scale_p         (scale),
  offset_p        (offset),
  fixed_p         (True),
  autoScale_p     (False),
  scaleColumn_p   (0),
  offsetColumn_p  (0),
  rwScaleColumn_p (0),
  rwOffsetColumn_p(0)
{}

CompressComplex::CompressComplex (const String& sourceColumnName,
				  const String& targetColumnName,
				  const String& scaleColumnName,
				  const String& offsetColumnName,
				  Bool autoScale)
: BaseMappedArrayEngine<Complex,Int> (sourceColumnName, targetColumnName),
  scaleName_p     (scaleColumnName),
  offsetName_p    (offsetColumnName),
  scale_p         (0.0),
  offset_p        (0.0),
  fixed_p         (False),
  autoScale_p     (autoScale),
  scaleColumn_p   (0),
  offsetColumn_p  (0),
  rwScaleColumn_p (0),
  rwOffsetColumn_p(0)
{}

CompressComplex::CompressComplex (const Record& spec)
: BaseMappedArrayEngine<Complex,Int> (),
  scale_p         (1.0),
  offset_p        (0.0),
  fixed_p         (True),
  autoScale_p     (False),
  scaleColumn_p   (0),
  offsetColumn_p  (0),
  rwScaleColumn_p (0),
  rwOffsetColumn_p(0)
{
  if (spec.isDefined("SOURCENAME")  &&  spec.isDefined("TARGETNAME")) {
    setNames (spec.asString("SOURCENAME"), spec.asString("TARGETNAME"));
    if (spec.isDefined("SCALE")  &&  spec.isDefined("OFFSET")) {
      spec.get ("SCALE", scale_p);
      spec.get ("OFFSET", offset_p);
    } else {
      spec.get ("SCALENAME", scaleName_p);
      spec.get ("OFFSETNAME", offsetName_p);
      fixed_p = False;
    }
    if (spec.isDefined("AUTOSCALE")) {
      spec.get ("AUTOSCALE", autoScale_p);
    }
  }
}

CompressComplex::CompressComplex (const CompressComplex& that)
: BaseMappedArrayEngine<Complex,Int> (that),
  scaleName_p     (that.scaleName_p),
  offsetName_p    (that.offsetName_p),
  scale_p         (that.scale_p),
  offset_p        (that.offset_p),
  fixed_p         (that.fixed_p),
  autoScale_p     (that.autoScale_p),
  scaleColumn_p   (0),
  offsetColumn_p  (0),
  rwScaleColumn_p (0),
  rwOffsetColumn_p(0)
{}

CompressComplex::~CompressComplex()
{
  delete scaleColumn_p;
  delete offsetColumn_p;
  delete rwScaleColumn_p;
  delete rwOffsetColumn_p;
}

//# Clone the engine object.
DataManager* CompressComplex::clone() const
{
  return new CompressComplex (*this);
}


//# Return the type name of the engine (i.e. its class name).
String CompressComplex::dataManagerType() const
{
  return className();
}
//# Return the class name.
//# Get the data type names using class ValType.
String CompressComplex::className()
{
  return "CompressComplex";
}

String CompressComplex::dataManagerName() const
{
  return sourceName();
}

Record CompressComplex::dataManagerSpec() const
{
  Record spec;
  spec.define ("SOURCENAME", sourceName());
  spec.define ("TARGETNAME", targetName());
  if (fixed_p) {
    spec.define ("SCALE", scale_p);
    spec.define ("OFFSET", offset_p);
  } else {
    spec.define ("SCALENAME", scaleName_p);
    spec.define ("OFFSETNAME", offsetName_p);
  }
  spec.define ("AUTOSCALE", autoScale_p);
  return spec;
}

DataManager* CompressComplex::makeObject (const String&, const Record& spec)
{
  return new CompressComplex(spec);
}
void CompressComplex::registerClass()
{
  DataManager::registerCtor (className(), makeObject);
}


void CompressComplex::create (uInt initialNrrow)
{
  BaseMappedArrayEngine<Complex,Int>::create (initialNrrow);
  // Store the various parameters as keywords in this column.
  TableColumn thisCol (table(), sourceName());
  thisCol.rwKeywordSet().define ("_CompressComplex_Scale",      scale_p);
  thisCol.rwKeywordSet().define ("_CompressComplex_Offset",     offset_p);
  thisCol.rwKeywordSet().define ("_CompressComplex_ScaleName",  scaleName_p);
  thisCol.rwKeywordSet().define ("_CompressComplex_OffsetName", offsetName_p);
  thisCol.rwKeywordSet().define ("_CompressComplex_Fixed",      fixed_p);
  thisCol.rwKeywordSet().define ("_CompressComplex_AutoScale",  autoScale_p);
}

void CompressComplex::prepare()
{
  BaseMappedArrayEngine<Complex,Int>::prepare1();
  ROTableColumn thisCol (table(), sourceName());
  thisCol.keywordSet().get ("_CompressComplex_Scale",      scale_p);
  thisCol.keywordSet().get ("_CompressComplex_Offset",     offset_p);
  thisCol.keywordSet().get ("_CompressComplex_ScaleName",  scaleName_p);
  thisCol.keywordSet().get ("_CompressComplex_OffsetName", offsetName_p);
  thisCol.keywordSet().get ("_CompressComplex_Fixed",      fixed_p);
  thisCol.keywordSet().get ("_CompressComplex_AutoScale",  autoScale_p);
  //# Allocate column objects to get scale and offset.
  if (! fixed_p) {
    scaleColumn_p = new ROScalarColumn<Float> (table(), scaleName_p);
    offsetColumn_p = new ROScalarColumn<Float> (table(), offsetName_p);
    if (autoScale_p  &&  isWritable()) {
      rwScaleColumn_p = new ScalarColumn<Float> (table(), scaleName_p);
      rwOffsetColumn_p = new ScalarColumn<Float> (table(), offsetName_p);
    }
  }
  // Do this at the end, because it might call addRow.
  BaseMappedArrayEngine<Complex,Int>::prepare2();
}

void CompressComplex::reopenRW()
{
  // Create the writable ArrayColumn object if it does not exist
  // yet and if the column is writable now.
  if (!fixed_p  &&  rwScaleColumn_p == 0) {
    if (autoScale_p  &&  isWritable()) {
      rwScaleColumn_p = new ScalarColumn<Float> (table(), scaleName_p);
      rwOffsetColumn_p = new ScalarColumn<Float> (table(), offsetName_p);
    }
  }
}

void CompressComplex::addRowInit (uInt startRow, uInt nrrow)
{
  BaseMappedArrayEngine<Complex,Int>::addRowInit (startRow, nrrow);
  if (autoScale_p) {
    for (uInt i=0; i<nrrow; i++) {
      rwScaleColumn_p->put (startRow++, 0.);
    }
  }
}

// Find minimum and maximum.
void CompressComplex::findMinMax (Float& minVal, Float& maxVal,
				  const Array<Complex>& array) const
{
  setNaN (minVal);
  setNaN (maxVal);
  Bool deleteIt;
  const Complex* data = array.getStorage (deleteIt);
  const uInt nr = array.nelements();
  Bool firstTime = True;
  for (uInt i=0; i<nr; i++) {
    if (! isNaN (data[i])) {
      Float tmp = data[i].real();
      if (firstTime) {
	minVal = tmp;
	maxVal = tmp;
	firstTime = False;
      }
      if (tmp < minVal) {
	minVal = tmp;
      } else if (tmp > maxVal) {
	maxVal = tmp;
      }
      tmp = data[i].imag();
      if (tmp < minVal) {
	minVal = tmp;
      } else if (tmp > maxVal) {
	maxVal = tmp;
      }
    }
  }
  array.freeStorage (data, deleteIt);
}

// Find minimum and maximum.
void CompressComplex::makeScaleOffset (Float& scale, Float& offset,
				       Float minVal, Float maxVal) const
{
  if (isNaN (minVal)) {
    scale = 0;
    offset = 0;
  } else {
    if (minVal == maxVal) {
      scale = 1;
    } else {
      scale = (maxVal - minVal) / 65534;
    }
    offset = (maxVal + minVal) / 2;
  }
}

// Scale/offset an array for get.
void CompressComplex::scaleOnGet (Float scale, Float offset,
				  Array<Complex>& array,
				  const Array<Int>& target)
{
  Bool deleteIn, deleteOut;
  Complex* out = array.getStorage (deleteOut);
  const Int* in = target.getStorage (deleteIn);
  const uInt nr = array.nelements();
  for (uInt i=0; i<nr; i++) {
    Int r = in[i] / 65536;
    if (r == -32768) {
      setNaN (out[i]);
    } else {
      Int im = in[i] - r*65536;
      if (im < -32768) {
	r  -= 1;
	im += 65536;
      } else if (im >= 32768) {
	r  += 1;
	im -= 65536;
      }
      out[i] = Complex (r * scale + offset, im * scale + offset);
    }
  }
  target.freeStorage (in, deleteIn);
  array.putStorage (out, deleteOut);
}

// Scale/offset an array for put.
void CompressComplex::scaleOnPut (Float scale, Float offset,
				  const Array<Complex>& array,
				  Array<Int>& target)
{
  Bool deleteIn, deleteOut;
  const Complex* in = array.getStorage (deleteIn);
  Int* out = target.getStorage (deleteOut);
  const uInt nr = array.nelements();
  for (uInt i=0; i<nr; i++) {
    if (isNaN (in[i])) {
      out[i] = -32768 * 65536;
    } else {
      Short s;
      Float tmp = (in[i].real() - offset) / scale;
      if (tmp < 0) {
	s = short (ceil(tmp - 0.5));
      } else {
	s = short (floor(tmp + 0.5));
      }
      Int r = int(s) * 65536;
      tmp = (in[i].imag() - offset) / scale;
      if (tmp < 0) {
	s = short (ceil(tmp - 0.5));
      } else {
	s = short (floor(tmp + 0.5));
      }
      out[i] = r + s;
    }
  }
  array.freeStorage (in, deleteIn);
  target.putStorage (out, deleteOut);
}


void CompressComplex::scaleColumnOnGet (Array<Complex>& array,
					const Array<Int>& target)
{
  if (fixed_p) {
    scaleOnGet (scale_p, offset_p, array, target);
  }else{
    ArrayIterator<Complex> arrayIter (array, array.ndim() - 1);
    ReadOnlyArrayIterator<Int> targetIter (target, target.ndim() - 1);
    uInt rownr = 0;
    while (! arrayIter.pastEnd()) {
      scaleOnGet (getScale(rownr), getOffset(rownr),
		  arrayIter.array(), targetIter.array());
      rownr++;
      arrayIter.next();
      targetIter.next();
    }
  }
}

void CompressComplex::scaleColumnOnPut (const Array<Complex>& array,
					Array<Int>& target)
{
  if (fixed_p) {
    scaleOnPut (scale_p, offset_p, array, target);
  }else{
    ReadOnlyArrayIterator<Complex> arrayIter (array, array.ndim() - 1);
    ArrayIterator<Int> targetIter (target, target.ndim() - 1);
    uInt rownr = 0;
    while (! arrayIter.pastEnd()) {
      scaleOnPut (getScale(rownr), getOffset(rownr),
		  arrayIter.array(), targetIter.array());
      rownr++;
      arrayIter.next();
      targetIter.next();
    }
  }
}


void CompressComplex::getArray (uInt rownr, Array<Complex>& array)
{
  if (! array.shape().isEqual (buffer_p.shape())) {
    buffer_p.resize (array.shape());
  }
  roColumn().baseGet (rownr, buffer_p);
  scaleOnGet (getScale(rownr), getOffset(rownr), array, buffer_p);
}

void CompressComplex::putArray (uInt rownr, const Array<Complex>& array)
{
  if (! array.shape().isEqual (buffer_p.shape())) {
    buffer_p.resize (array.shape());
  }
  if (! autoScale_p) {
    scaleOnPut (getScale(rownr), getOffset(rownr), array, buffer_p);
  } else {
    Float minVal, maxVal;
    findMinMax (minVal, maxVal, array);
    Float scale, offset;
    makeScaleOffset (scale, offset, minVal, maxVal);
    rwScaleColumn_p->put (rownr, scale);
    rwOffsetColumn_p->put (rownr, offset);
    scaleOnPut (scale, offset, array, buffer_p);
  }
  rwColumn().basePut (rownr, buffer_p);
}

void CompressComplex::getSlice (uInt rownr, const Slicer& slicer,
				Array<Complex>& array)
{
  if (! array.shape().isEqual (buffer_p.shape())) {
    buffer_p.resize (array.shape());
  }
  roColumn().getSlice (rownr, slicer, buffer_p);
  scaleOnGet (getScale(rownr), getOffset(rownr), array, buffer_p);
}

void CompressComplex::putPart (uInt rownr, const Slicer& slicer,
			       const Array<Complex>& array,
			       Float scale, Float offset)
{
  if (! array.shape().isEqual (buffer_p.shape())) {
    buffer_p.resize (array.shape());
  }
  scaleOnPut (scale, offset, array, buffer_p);
  rwColumn().putSlice (rownr, slicer, buffer_p);
}

void CompressComplex::putFullPart (uInt rownr, const Slicer& slicer,
				   Array<Complex>& fullArray,
				   const Array<Complex>& partArray,
				   Float minVal, Float maxVal)
{
  Array<Complex> subarr = fullArray(slicer.start(), slicer.end(),
				  slicer.stride());
  subarr = partArray;
  Float scale, offset;
  makeScaleOffset (scale, offset, minVal, maxVal);
  rwScaleColumn_p->put (rownr, scale);
  rwOffsetColumn_p->put (rownr, offset);
  if (! fullArray.shape().isEqual (buffer_p.shape())) {
    buffer_p.resize (fullArray.shape());
  }
  scaleOnPut (scale, offset, fullArray, buffer_p);
  rwColumn().basePut (rownr, buffer_p);
}

void CompressComplex::putSlice (uInt rownr, const Slicer& slicer,
				const Array<Complex>& array)
{
  // If the slice is the entire array, write it as such.
  IPosition shp = shape(rownr);
  if (shp.isEqual (array.shape())) {
    CompressComplex::putArray (rownr, array);
  } else {
    // Get current scale and offset.
    // If no autoscaling, write the part immediately.
    Float scale = getScale(rownr);
    Float offset = getOffset(rownr);
    if (! autoScale_p) {
      putPart (rownr, slicer, array, scale, offset);
    } else {
      // Determine min/max of new slice.
      // scale==0 means that no array data was written yet.
      // In that case initialize array to NaN if the slice has valid data.
      Float minValArr, maxValArr;
      findMinMax (minValArr, maxValArr, array);
      if (scale == 0) {
	if (! isNaN(minValArr)) {
	  Array<Complex> arr(shp);
	  Complex val;
	  setNaN (val);
	  arr = val;
	  putFullPart (rownr, slicer, arr, array, minValArr, maxValArr);
	}
      } else {
	// Valid data in row.
	// Writing the part will do if no valid data in it or if
	// its min/max is within the current min/max.
	// Otherwise we have to rescale using new min/max.
	Float maxValRow = offset + scale*65534/2;
	Float minValRow = offset - scale*65534/2;
	if (isNaN(minValArr)
	    ||  (minValArr >= minValRow  &&  maxValArr <= maxValRow)) {
	  putPart (rownr, slicer, array, scale, offset);
	} else {
	  Array<Complex> arr(shp);
	  CompressComplex::getArray (rownr, arr);
	  putFullPart (rownr, slicer, arr, array,
		       min(minValRow, minValArr),
		       max(maxValRow, maxValArr));
	}
      }
    }
  }
}

void CompressComplex::getArrayColumn (Array<Complex>& array)
{
  Array<Int> target(array.shape());
  roColumn().getColumn (target);
  scaleColumnOnGet (array, target);
}
void CompressComplex::putArrayColumn (const Array<Complex>& array)
{
  Array<Int> target(array.shape());
  if (! autoScale_p) {
    scaleColumnOnPut (array, target);
    rwColumn().putColumn (target);
  } else {
    ReadOnlyArrayIterator<Complex> iter(array, array.ndim()-1);
    uInt nrrow = table().nrow();
    for (uInt rownr=0; rownr<nrrow; rownr++) {
      CompressComplex::putArray (rownr, iter.array());
      iter.next();
    }
  }
}

void CompressComplex::getColumnSlice (const Slicer& slicer,
				      Array<Complex>& array)
{
  Array<Int> target(array.shape());
  roColumn().getColumn (slicer, target);
  scaleColumnOnGet (array, target);
}

void CompressComplex::putColumnSlice (const Slicer& slicer,
				      const Array<Complex>& array)
{
  Array<Int> target(array.shape());
  if (! autoScale_p) {
    scaleColumnOnPut (array, target);
    rwColumn().putColumn (slicer, target);
  } else {
    ReadOnlyArrayIterator<Complex> iter(array, array.ndim()-1);
    uInt nrrow = table().nrow();
    for (uInt rownr=0; rownr<nrrow; rownr++) {
      CompressComplex::putSlice (rownr, slicer, iter.array());
      iter.next();
    }
  }
}
