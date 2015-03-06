/* $Id: SeqTable_multi_data.cpp 448566 2014-10-07 16:44:32Z ivanov $
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
 *   using the following specifications:
 *   'seqtable.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/seqtable/SeqTable_multi_data.hpp>
#include <objects/seqtable/CommonString_table.hpp>
#include <objects/seqtable/CommonBytes_table.hpp>
#include <objects/seqtable/Seq_table.hpp>
#include <serial/objhook.hpp>
#include <corelib/ncbi_param.hpp>

#include <util/bitset/ncbi_bitset.hpp>
#include <objects/seqtable/seq_table_exception.hpp>
#include <cmath>

#include <objects/seqtable/impl/delta_cache.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// constructor
CSeqTable_multi_data::CSeqTable_multi_data(void)
{
}


// destructor
CSeqTable_multi_data::~CSeqTable_multi_data(void)
{
}


DEFINE_STATIC_MUTEX(sx_PrepareMutex_multi_data);


void CSeqTable_multi_data::x_ResetCache(void)
{
    m_Cache.Reset();
}


CIntDeltaSumCache& CSeqTable_multi_data::x_GetIntDeltaCache(void) const
{
    CIntDeltaSumCache* info = m_Cache.GetNCPointerOrNull();
    if ( !info ) {
        m_Cache = info = new CIntDeltaSumCache(GetInt_delta().GetSize());
    }
    return *info;
}


/////////////////////////////////////////////////////////////////////////////
// CIntDeltaSumCache
/////////////////////////////////////////////////////////////////////////////

#define USE_DELTA_CACHE
const size_t CIntDeltaSumCache::kBlockSize = 128;


CIntDeltaSumCache::CIntDeltaSumCache(size_t size)
    : m_Blocks(new TValue[(size+kBlockSize-1)/kBlockSize]),
      m_BlocksFilled(0),
#ifdef USE_DELTA_CACHE
      m_CacheBlockInfo(new TValue[kBlockSize]),
#endif
      m_CacheBlockIndex(size_t(0)-1)
{
}


CIntDeltaSumCache::~CIntDeltaSumCache(void)
{
}


inline
CIntDeltaSumCache::TValue
CIntDeltaSumCache::x_GetDeltaSum2(const TDeltas& deltas,
                                  size_t block_index,
                                  size_t block_offset)
{
#ifdef USE_DELTA_CACHE
    _ASSERT(block_index <= m_BlocksFilled);
    if ( block_index != m_CacheBlockIndex ) {
        size_t size = deltas.GetSize();
        size_t block_pos = block_index*kBlockSize;
        _ASSERT(block_pos < size);
        size_t block_size = min(kBlockSize, size-block_pos);
        _ASSERT(block_offset < block_size);
        TValue sum = block_index == 0? 0: m_Blocks[block_index-1];
        for ( size_t i = 0; i < block_size; ++i ) {
            int v;
            if ( deltas.TryGetInt(block_pos+i, v) ) {
                sum += v;
            }
            m_CacheBlockInfo[i] = sum;
        }
        m_CacheBlockIndex = block_index;
        if ( block_index == m_BlocksFilled ) {
            m_Blocks[block_index] = sum;
            m_BlocksFilled = block_index+1;
        }
    }
    return m_CacheBlockInfo[block_offset];
#else
    size_t size = deltas.GetSize();
    size_t block_pos = block_index*kBlockSize;
    _ASSERT(block_pos < size);
    size_t block_size = min(kBlockSize, size-block_pos);
    _ASSERT(block_offset < block_size);
    TValue sum = block_index == 0? 0: m_Blocks[block_index-1];
    for ( size_t i = 0; i <= block_offset; ++i ) {
        int v;
        if ( deltas.TryGetInt(block_pos+i, v) ) {
            sum += v;
        }
    }
    if ( block_index == m_BlocksFilled ) {
        TValue sum2 = sum;
        for ( size_t i = block_offset+1; i < block_size; ++i ) {
            int v;
            if ( deltas.TryGetInt(block_pos+i, v) ) {
                sum2 += v;
            }
        }
        m_Blocks[block_index] = sum2;
        m_BlocksFilled = block_index+1;
    }
    return sum;
#endif
}


CIntDeltaSumCache::TValue
CIntDeltaSumCache::GetDeltaSum(const TDeltas& deltas,
                               size_t index)
{
    _ASSERT(index < deltas.GetSize());
    size_t block_index  = index / kBlockSize;
    size_t block_offset = index % kBlockSize;
    while ( block_index >= m_BlocksFilled ) {
        x_GetDeltaSum2(deltas, m_BlocksFilled, 0);
    }
    return x_GetDeltaSum2(deltas, block_index, block_offset);
}


/////////////////////////////////////////////////////////////////////////////


bool CSeqTable_multi_data::TryGetBool(size_t row, bool& v) const
{
    if ( IsBit() ) {
        const TBit& bits = GetBit();
        size_t i = row/8;
        if ( i >= bits.size() ) {
            return false;
        }
        size_t j = row%8;
        Uint1 bb = bits[i];
        v = ((bb<<j)&0x80) != 0;
        return true;
    }
    else if ( IsBit_bvector() ) {
        const bm::bvector<>& bv = GetBit_bvector().GetBitVector();
        if ( row >= bv.size() ) {
            return false;
        }
        v = bv.get_bit(row);
        return true;
    }
    else if ( IsInt() ) {
        const TInt& arr = GetInt();
        if ( row >= arr.size() ) {
            return false;
        }
        v = arr[row] != 0;
        return true;
    }
    NCBI_THROW(CSeqTableException, eIncompatibleRowType,
               "CSeqTable_multi_data::TryGetBool(): "
               "data cannot be converted to bool");
}


bool CSeqTable_multi_data::TryGetInt(size_t row, int& v) const
{
    if ( IsInt() ) {
        const TInt& arr = GetInt();
        if ( row >= arr.size() ) {
            return false;
        }
        v = arr[row];
        return true;
    }
    else if ( IsInt_delta() ) {
        const CSeqTable_multi_data& deltas = GetInt_delta();
        if ( row >= deltas.GetSize() ) {
            return false;
        }
        CMutexGuard guard(sx_PrepareMutex_multi_data);
        v = x_GetIntDeltaCache().GetDeltaSum(deltas, row);
        return true;
    }
    else if ( IsInt_scaled() ) {
        const TInt_scaled& scaled = GetInt_scaled();
        if ( !scaled.GetData().TryGetInt(row, v) ) {
            return false;
        }
        v = v*scaled.GetMul()+scaled.GetAdd();
        return true;
    }
    // up cast from bool
    bool v1;
    try {
        if ( !TryGetBool(row, v1) ) {
            return false;
        }
        v = v1;
        return true;
    }
    catch ( CSeqTableException& exc ) {
        if ( exc.GetErrCode() == exc.eIncompatibleRowType ) {
            NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                       "CSeqTable_multi_data::TryGetInt(): "
                       "data cannot be converted to int");
        }
        throw;
    }
}


bool CSeqTable_multi_data::TryGetReal(size_t row, double& v) const
{
    if ( IsReal() ) {
        const TReal& arr = GetReal();
        if ( row >= arr.size() ) {
            return false;
        }
        v = arr[row];
        return true;
    }
    else if ( IsReal_scaled() ) {
        const TReal_scaled& scaled = GetReal_scaled();
        if ( !scaled.GetData().TryGetReal(row, v) ) {
            return false;
        }
        v = v*scaled.GetMul()+scaled.GetAdd();
        return true;
    }
    // up cast from int
    int v1;
    try {
        if ( !TryGetInt(row, v1) ) {
            return false;
        }
        v = v1;
        return true;
    }
    catch ( CSeqTableException& exc ) {
        if ( exc.GetErrCode() == exc.eIncompatibleRowType ) {
            NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                       "CSeqTable_multi_data::TryGetReal() "
                       "data cannot be converted to real");
        }
        throw;
    }
}


const string* CSeqTable_multi_data::GetStringPtr(size_t row) const
{
    if ( IsString() ) {
        const CSeqTable_multi_data::TString& arr = GetString();
        if ( row < arr.size() ) {
            return &arr[row];
        }
    }
    else if ( IsCommon_string() ) {
        const CCommonString_table& common = GetCommon_string();
        const CCommonString_table::TIndexes& indexes = common.GetIndexes();
        if ( row < indexes.size() ) {
            const CCommonString_table::TStrings& arr = common.GetStrings();
            size_t arr_index = indexes[row];
            if ( arr_index < arr.size() ) {
                return &arr[arr_index];
            }
        }
    }
    else {
        NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                   "CSeqTable_multi_data::GetStringPtr() "
                   "data cannot be converted to string");
    }
    return 0;
}


const vector<char>* CSeqTable_multi_data::GetBytesPtr(size_t row) const
{
    if ( IsBytes() ) {
        const CSeqTable_multi_data::TBytes& arr = GetBytes();
        if ( row < arr.size() ) {
            return arr[row];
        }
    }
    else if ( IsCommon_bytes() ) {
        const CCommonBytes_table& common = GetCommon_bytes();
        const CCommonBytes_table::TIndexes& indexes = common.GetIndexes();
        if ( row < indexes.size() ) {
            const CCommonBytes_table::TBytes& arr = common.GetBytes();
            size_t arr_index = indexes[row];
            if ( arr_index < arr.size() ) {
                return arr[arr_index];
            }
        }
    }
    else {
        NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                   "CSeqTable_multi_data::GetBytesPtr() "
                   "data cannot be converted to OCTET STRING");
    }
    return 0;
}


size_t CSeqTable_multi_data::GetSize(void) const
{
    switch ( Which() ) {
    case e_Int:
        return GetInt().size();
    case e_Real:
        return GetReal().size();
    case e_String:
        return GetString().size();
    case e_Bytes:
        return GetBytes().size();
    case e_Common_string:
        return GetCommon_string().GetIndexes().size();
    case e_Common_bytes:
        return GetCommon_bytes().GetIndexes().size();
    case e_Bit:
        return GetBit().size()*8;
    case e_Loc:
        return GetLoc().size();
    case e_Id:
        return GetId().size();
    case e_Interval:
        return GetInterval().size();
    case e_Bit_bvector: 
        return GetBit_bvector().GetSize();
    case e_Int_delta:
        return GetInt_delta().GetSize();
    case e_Int_scaled:
        return GetInt_scaled().GetData().GetSize();
    case e_Real_scaled:
        return GetReal_scaled().GetData().GetSize();
    default:
        break;
    }
    return 0;
}


NCBI_PARAM_DECL(bool, OBJECTS, SEQ_TABLE_RESERVE);
NCBI_PARAM_DEF_EX(bool, OBJECTS, SEQ_TABLE_RESERVE, true,
                  eParam_NoThread, OBJECTS_SEQ_TABLE_RESERVE);

void CSeqTable_multi_data::CReserveHook::PreReadChoiceVariant(
    CObjectIStream& in,
    const CObjectInfoCV& variant)
{
    static CSafeStatic<NCBI_PARAM_TYPE(OBJECTS, SEQ_TABLE_RESERVE)> s_Reserve;

    if ( !s_Reserve->Get() ) {
        return;
    }
    if ( CSeq_table* table = CType<CSeq_table>::GetParent(in, 5, 2) ) {
        size_t size = table->GetNum_rows();
        CSeqTable_multi_data* data =
            CType<CSeqTable_multi_data>::Get(variant.GetChoiceObject());
        switch ( variant.GetVariantIndex() ) {
        case e_Int:
            data->SetInt().reserve(size);
            break;
        case e_Real:
            data->SetReal().reserve(size);
            break;
        case e_String:
            data->SetString().reserve(size);
            break;
        case e_Bytes:
            data->SetBytes().reserve(size);
            break;
        case e_Common_string:
            data->SetCommon_string().SetIndexes().reserve(size);
            break;
        case e_Common_bytes:
            data->SetCommon_bytes().SetIndexes().reserve(size);
            break;
        case e_Bit:
            data->SetBit().reserve((size+7)/8);
            break;
        case e_Loc:
            data->SetLoc().reserve(size);
            break;
        case e_Id:
            data->SetId().reserve(size);
            break;
        case e_Interval:
            data->SetInterval().reserve(size);
            break;
        default:
            break;
        }
    }
}


void CSeqTable_multi_data::ChangeTo(E_Choice type)
{
    if ( Which() == type ) {
        return;
    }
    switch ( type ) {
    case e_Int:
        ChangeToInt();
        break;
    case e_Real:
        ChangeToReal();
        break;
    case e_String:
        ChangeToString();
        break;
    case e_Common_string:
        ChangeToCommon_string();
        break;
    case e_Bytes:
        ChangeToBytes();
        break;
    case e_Common_bytes:
        ChangeToCommon_bytes();
        break;
    case e_Bit:
        ChangeToBit();
        break;
    case e_Int_delta:
        ChangeToInt_delta();
        break;
    case e_Bit_bvector:
        ChangeToBit_bvector();
        break;
    case e_Int_scaled: // scaling requires extra parameters
        NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                   "CSeqTable_multi_data::ChangeTo(e_Int_scaled): "
                   "scaling parameters are unknown");
    case e_Real_scaled: // scaling requires extra parameters
        NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                   "CSeqTable_multi_data::ChangeTo(e_Real_scaled): "
                   "scaling parameters are unknown");
    default:
        NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                   "CSeqTable_multi_data::ChangeTo(): "
                   "requested multi-data type is invalid");
    }
}


void CSeqTable_multi_data::ChangeToString(const string* omitted_value)
{
    if ( IsString() ) {
        return;
    }
    if ( IsCommon_string() ) {
        const CCommonString_table& common = GetCommon_string();
        const CCommonString_table::TIndexes& indexes = common.GetIndexes();
        size_t size = indexes.size();
        TString arr;
        arr.reserve(size);
        const CCommonString_table::TStrings& src = common.GetStrings();
        ITERATE ( CCommonString_table::TIndexes, it, indexes ) {
            size_t index = *it;
            if ( index < src.size() ) {
                arr.push_back(src[index]);
            }
            else if ( omitted_value ) {
                arr.push_back(*omitted_value);
            }
            else {
                NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                           "CSeqTable_multi_data::ChangeToString(): "
                           "common string table is sparse");
            }
        }
        swap(SetString(), arr);
        return;
    }
    NCBI_THROW(CSeqTableException, eIncompatibleRowType,
               "CSeqTable_multi_data::ChangeToString(): "
               "requested mult-data type is invalid");
}


void CSeqTable_multi_data::ChangeToCommon_string(const string* omit_value)
{
    if ( IsCommon_string() ) {
        return;
    }
    if ( IsString() ) {
        CRef<CCommonString_table> common(new CCommonString_table);
        CCommonString_table::TIndexes& indexes = common->SetIndexes();
        CCommonString_table::TStrings& arr = common->SetStrings();
        const TString& src = GetString();
        indexes.reserve(src.size());
        typedef map<string, size_t> TIndexMap;
        TIndexMap index_map;
        if ( omit_value ) {
            index_map[*omit_value] = -1;
        }
        ITERATE ( TString, it, src ) {
            const string& key = *it;
            TIndexMap::iterator iter = index_map.lower_bound(key);
            if ( iter == index_map.end() || iter->first != key ) {
                iter = index_map.insert(iter, TIndexMap::value_type(key, arr.size()));
                arr.push_back(key);
            }
            indexes.push_back(iter->second);
        }
        SetCommon_string(*common);
        return;
    }
    NCBI_THROW(CSeqTableException, eIncompatibleRowType,
               "CSeqTable_multi_data::ChangeToCommon_string(): "
               "requested mult-data type is invalid");
}


void CSeqTable_multi_data::ChangeToBytes(const TBytesValue* omitted_value)
{
    if ( IsBytes() ) {
        return;
    }
    if ( IsCommon_bytes() ) {
        const CCommonBytes_table& common = GetCommon_bytes();
        const CCommonBytes_table::TIndexes& indexes = common.GetIndexes();
        size_t size = indexes.size();
        TBytes arr;
        arr.reserve(size);
        const CCommonBytes_table::TBytes& src = common.GetBytes();
        ITERATE ( CCommonBytes_table::TIndexes, it, indexes ) {
            size_t index = *it;
            const TBytesValue* value;
            if ( index < src.size() ) {
                value = src[index];
            }
            else if ( omitted_value ) {
                value = omitted_value;
            }
            else {
                NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                           "CSeqTable_multi_data::ChangeToBytes(): "
                           "common bytes table is sparse");
            }
            arr.push_back(new TBytesValue(*value));
        }
        swap(SetBytes(), arr);
        return;
    }
    NCBI_THROW(CSeqTableException, eIncompatibleRowType,
               "CSeqTable_multi_data::ChangeToBytes(): "
               "requested mult-data type is invalid");
}


void CSeqTable_multi_data::ChangeToCommon_bytes(const TBytesValue* omit_value)
{
    if ( IsCommon_bytes() ) {
        return;
    }
    if ( IsBytes() ) {
        CRef<CCommonBytes_table> common(new CCommonBytes_table);
        CCommonBytes_table::TIndexes& indexes = common->SetIndexes();
        CCommonBytes_table::TBytes& arr = common->SetBytes();
        const TBytes& src = GetBytes();
        indexes.reserve(src.size());
        typedef map<const TBytesValue*, size_t, PPtrLess<const TBytesValue*> > TIndexMap;
        TIndexMap index_map;
        if ( omit_value ) {
            index_map[omit_value] = -1;
        }
        ITERATE ( TBytes, it, src ) {
            const TBytesValue* key = *it;
            TIndexMap::iterator iter = index_map.lower_bound(key);
            if ( iter == index_map.end() || *iter->first != *key ) {
                iter = index_map.insert(iter, TIndexMap::value_type(key, arr.size()));
                arr.push_back(new TBytesValue(*key));
            }
            indexes.push_back(iter->second);
        }
        SetCommon_bytes(*common);
        return;
    }
    NCBI_THROW(CSeqTableException, eIncompatibleRowType,
               "CSeqTable_multi_data::ChangeToBytes(): "
               "requested mult-data type is invalid");
}


void CSeqTable_multi_data::ChangeToReal_scaled(double mul, double add)
{
    if ( IsReal_scaled() ) {
        return;
    }
    TReal arr;
    if ( IsReal() ) {
        // in-place
        swap(arr, SetReal());
        NON_CONST_ITERATE ( TReal, it, arr ) {
            TReal::value_type value = *it;
            value -= add;
            *it = value/mul;
        }
    }
    else {
        for ( size_t row = 0; ; ++row ) {
            TReal::value_type value;
            if ( !TryGetReal(row, value) ) {
                break;
            }
            value -= add;
            arr.push_back(value/mul);
        }
    }
    swap(SetReal_scaled().SetData().SetReal(), arr);
}


void CSeqTable_multi_data::ChangeToInt_scaled(int mul, int add)
{
    if ( IsInt_scaled() ) {
        return;
    }
    TInt arr;
    if ( IsInt() ) {
        // in-place
        swap(arr, SetInt());
        NON_CONST_ITERATE ( TInt, it, arr ) {
            TInt::value_type value = *it;
            value -= add;
            if ( value % mul != 0 ) {
                // restore already scaled values
                while ( it != arr.begin() ) {
                    TInt::value_type value = *--it;
                    *it = value*mul+add;
                }
                swap(arr, SetInt());
                NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                           "CSeqTable_multi_data::ChangeToInt_scaled(): "
                           "value is not round for scaling");
            }
            *it = value/mul;
        }
    }
    else {
        for ( size_t row = 0; ; ++row ) {
            TInt::value_type value;
            if ( !TryGetInt(row, value) ) {
                break;
            }
            value -= add;
            if ( value % mul != 0 ) {
                NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                           "CSeqTable_multi_data::ChangeToInt_scaled(): "
                           "value is not round for scaling");
            }
            arr.push_back(value/mul);
        }
    }
    swap(SetInt_scaled().SetData().SetInt(), arr);
}


void CSeqTable_multi_data::ChangeToInt_delta(void)
{
    if ( IsInt_delta() ) {
        return;
    }
    TInt arr;
    if ( IsInt() ) {
        // in-place
        swap(arr, SetInt());
        TInt::value_type prev_value = 0;
        NON_CONST_ITERATE ( TInt, it, arr ) {
            TInt::value_type value = *it;
            *it = value - prev_value;
            prev_value = value;
        }
    }
    else {
        TInt::value_type prev_value = 0;
        for ( size_t row = 0; ; ++row ) {
            TInt::value_type value;
            if ( !TryGetInt(row, value) ) {
                break;
            }
            arr.push_back(value-prev_value);
            prev_value = value;
        }
    }
    Reset();
    swap(SetInt_delta().SetInt(), arr);
}


void CSeqTable_multi_data::ChangeToInt(void)
{
    if ( IsInt() ) {
        return;
    }
    if ( IsInt_delta() ) {
        TInt arr;
        size_t size = GetSize();
        arr.reserve(size);
        for ( size_t row = 0; row < size; ++row ) {
            int value;
            if ( !TryGetInt(row, value) ) {
                break;
            }
            arr.push_back(value);
        }
        SetInt().swap(arr);
        return;
    }
    TInt arr;
    if ( IsReal() || IsReal_scaled() ) {
        for ( size_t row = 0; ; ++row ) {
            double value;
            if ( !TryGetReal(row, value) ) {
                break;
            }
            TInt::value_type int_value =
                TInt::value_type(value > 0? floor(value+.5): ceil(value-.5));
            arr.push_back(int_value);
        }
    }
    else {
        for ( size_t row = 0; ; ++row ) {
            TInt::value_type value;
            if ( !TryGetInt(row, value) ) {
                break;
            }
            arr.push_back(value);
        }
    }
    Reset();
    swap(SetInt(), arr);
}


void CSeqTable_multi_data::ChangeToReal(void)
{
    if ( IsReal() ) {
        return;
    }
    TReal arr;
    for ( size_t row = 0; ; ++row ) {
        double value;
        if ( !TryGetReal(row, value) ) {
            break;
        }
        arr.push_back(TInt::value_type(value));
    }
    Reset();
    swap(SetReal(), arr);
}


void CSeqTable_multi_data::ChangeToBit(void)
{
    if ( IsBit() ) {
        return;
    }
    TBit arr;
    if ( IsBit_bvector() ) {
        const bm::bvector<>& bv = GetBit_bvector().GetBitVector();
        arr.reserve((bv.size()+7)/8);
        if ( bv.any() ) {
            size_t last_byte_index = 0;
            Uint1 last_byte = 0;
            bm::id_t i = bv.get_first();
            do {
                size_t byte_index = i / 8;
                if ( byte_index != last_byte_index ) {
                    arr.resize(last_byte_index);
                    arr.push_back(last_byte);
                    last_byte_index = byte_index;
                    last_byte = 0;
                }
                size_t bit_index = i % 8;
                last_byte |= 0x80 >> bit_index;
            } while ( (i=bv.get_next(i)) );
            if ( last_byte ) {
                arr.resize(last_byte_index);
                arr.push_back(last_byte);
            }
        }
        arr.resize((bv.size()+7)/8);
    }
    else if ( IsInt() ) {
        const TInt& src = GetInt();
        size_t size = src.size();
        arr.resize((size+7)/8);
        for ( size_t i = 0; i < size; ++i ) {
            if ( src[i] ) {
                arr[i/8] |= 0x80 >> i%8;
            }
        }
    }
    else {
        NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                   "CSeqTable_multi_data::ChangeToBit(): "
                   "requested mult-data type is invalid");
    }
    Reset();
    swap(SetBit(), arr);
}


void CSeqTable_multi_data::ChangeToBit_bvector(void)
{
    if ( IsBit_bvector() ) {
        return;
    }
    AutoPtr<bm::bvector<> > bv(new bm::bvector<>(GetSize()));
    if ( IsBit() ) {
        const TBit& src = GetBit();
        size_t size = src.size();
        for ( size_t i = 0; i < size; ++i ) {
            if ( Uint1 b = src[i] ) {
                // the following cycle assumes that Uint1 is exactly 8-bit
                for ( size_t j = 0; b; ++j, b <<= 1 ) {
                    if ( b&0x80 ) {
                        bv->set_bit(i*8+j);
                    }
                }
            }
        }
    }
    else if ( IsInt() ) {
        const TInt& src = GetInt();
        size_t size = src.size();
        for ( size_t i = 0; i < size; ++i ) {
            if ( src[i] ) {
                bv->set_bit(i);
            }
        }
    }
    else {
        NCBI_THROW(CSeqTableException, eIncompatibleRowType,
                   "CSeqTable_multi_data::ChangeToBit_bvector(): "
                   "requested mult-data type is invalid");
    }
    bv->optimize();
    SetBit_bvector().SetBitVector(bv.release());
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE
