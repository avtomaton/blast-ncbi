/* $Id: Object_id.cpp 420652 2013-12-04 16:44:17Z vasilche $
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 * Author:  .......
 *
 * File Description:
 *   .......
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using specifications from the ASN data definition file
 *   'general.asn'.
 */

// standard includes

// generated includes
#include <ncbi_pch.hpp>
#include <objects/general/Object_id.hpp>
#include <corelib/ncbistd.hpp>
#include <corelib/ncbimtx.hpp>

// generated classes

#include <serial/objistr.hpp>

BEGIN_NCBI_SCOPE
BEGIN_objects_SCOPE // namespace ncbi::objects::


// destructor
CObject_id::~CObject_id(void)
{
    return;
}


// match for identity
bool CObject_id::Match(const CObject_id& oid2) const
{
    E_Choice type = Which();

    if ( type != oid2.Which() )
        return false;

    switch ( type ) {
    case e_Id:
        return GetId() == oid2.GetId();
    case e_Str:
        return PNocase().Equals(GetStr(), oid2.GetStr());
    default:
        return this == &oid2;
    }
}


CObject_id::E_Choice CObject_id::GetIdType(Int8& value) const
{
    switch ( Which() ) {
    case e_Id:
        value = GetId();
        return e_Id;
    case e_Str:
        value = NStr::StringToInt8(GetStr(), NStr::fConvErr_NoThrow);
        if ( !value ) {
            if ( errno ) {
                // not convertible to integer
                return CObject_id::e_Str;
            }
            // converted to 0
            if ( GetStr().size() != 1 ) {
                // leading zeroes are not allowed
                return CObject_id::e_Str;
            }
            // valid zero as a string
            return CObject_id::e_Id;
        }
        if ( value > 0 ) {
            // non-zero positive value
            if ( GetStr()[0] == '0' || GetStr()[0] == '+' ) {
                // redundant '+' or leading zeroes are not allowed
                value = 0;
                return CObject_id::e_Str;
            }
        }
        else {
            // non-zero negative value
            if ( GetStr()[1] == '0' ) {
                // leading zeroes are not allowed
                value = 0;
                return CObject_id::e_Str;
            }
        }
        // valid non-zero value as a string
        return CObject_id::e_Id;
    default:
        value = 0;
        return CObject_id::e_not_set;
    }
}


// match for identity
int CObject_id::Compare(const CObject_id& oid2) const
{
    Int8 value, value2;
    E_Choice type = GetIdType(value);
    E_Choice type2 = oid2.GetIdType(value2);
    if ( int diff = type - type2 ) {
        return diff;
    }
    switch ( type ) {
    case e_Id:
        return value < value2? -1: (value > value2);
    case e_Str:
        return PNocase().Compare(GetStr(), oid2.GetStr());
    default:
        return 0;
    }
}


// format contents into a stream
ostream& CObject_id::AsString(ostream &s) const
{
    switch ( Which() ) {
    case e_Id:
        s << GetId();
        break;
    case e_Str:
        s << GetStr(); // no Upcase() on output as per Ostell 7/2001 - Karl
        break;
    default:
        break;
    }
    return s;
}


CObject_id&
CReadSharedObjectIdHookBase::GetSharedObject_id(const string& id)
{
    CRef<CObject_id>& shared_id = m_MapByStr[id];
    if ( !shared_id ) {
        shared_id = new CObject_id;
        shared_id->SetStr(id);
    }
    return *shared_id;
}


CObject_id&
CReadSharedObjectIdHookBase::GetSharedObject_id(int id)
{
    CRef<CObject_id>& shared_id = m_MapByInt[id];
    if ( !shared_id ) {
        shared_id = new CObject_id;
        shared_id->SetId(id);
    }
    return *shared_id;
}


DEFINE_STATIC_FAST_MUTEX(s_SharedIdMutex);


CObject_id&
CReadSharedObjectIdHookBase::ReadSharedObject_id(CObjectIStream& in)
{
    CFastMutexGuard guard(s_SharedIdMutex);
    in.ReadObject(&m_Temp, m_Temp.GetTypeInfo());
    return GetSharedObject_id(m_Temp);
}


END_objects_SCOPE // namespace ncbi::objects::
END_NCBI_SCOPE
