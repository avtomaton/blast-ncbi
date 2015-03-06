/* $Id: Spliced_seg.cpp 430022 2014-03-21 14:07:20Z mozese2 $
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
 * Author:  Kamen Todorov
 *
 * File Description:
 *   User-defined methods for Spliced-seg.
 *
 * Remark:
 *   This code was originally generated by application DATATOOL
 *   using the following specifications:
 *   'seqalign.asn'.
 */

// standard includes
#include <ncbi_pch.hpp>

// generated includes
#include <objects/seqalign/Spliced_seg.hpp>

// generated classes

BEGIN_NCBI_SCOPE

BEGIN_objects_SCOPE // namespace ncbi::objects::

// destructor
CSpliced_seg::~CSpliced_seg(void)
{
}


ENa_strand 
CSpliced_seg::GetSeqStrand(TDim row) const 
{
    switch (row) {
    case 0:
        if (CanGetProduct_strand()) {
            return GetProduct_strand();
        } else {
            if ((*GetExons().begin())->CanGetProduct_strand()) {
                return (*GetExons().begin())->GetProduct_strand();
            } else {
                return eNa_strand_unknown;
            }
        }
        break;
    case 1:
        if (CanGetGenomic_strand()) {
            return GetGenomic_strand();
        } else {
            if ((*GetExons().begin())->CanGetGenomic_strand()) {
                return (*GetExons().begin())->GetGenomic_strand();
            } else {
                return eNa_strand_unknown;
            }
        }
        break;
    default:
        NCBI_THROW(CSeqalignException, eInvalidRowNumber,
                   "CSpliced_seg::GetSeqStrand(): Invalid row number");
    }
}

void CSpliced_seg::Validate(bool full_test) const
{
    bool prot = GetProduct_type() == eProduct_type_protein;

    TSeqRange product_position_limits = TSeqRange::GetWhole();
    if (IsSetProduct_length()) {
        TSeqPos product_length = GetProduct_length();
        product_position_limits.SetTo((prot ? product_length * 3 : product_length) -1);
    }
    if (IsSetPoly_a()) {
        if (prot) {
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                       "CSpliced_seg::Validate(): poly-a on a protein");
        }
        int poly_a = GetPoly_a();
        if (poly_a > 0 && TSeqPos(poly_a) > product_position_limits.GetTo()+1) {
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                       "CSpliced_seg::Validate(): poly-a > product-length");
        }
        if (CanGetProduct_strand() && GetProduct_strand() == eNa_strand_minus) {
            product_position_limits.SetFrom(poly_a+1);
        } else {
            product_position_limits.SetTo(poly_a-1);
        }
    }


    if (GetExons().empty()) {
        NCBI_THROW(CSeqalignException, eInvalidAlignment,
                   "CSpliced_seg::Validate(): Spiced-seg is empty (has no exons)");
    }

    ITERATE (CSpliced_seg::TExons, exon_it, GetExons()) {

        const CSpliced_exon& exon = **exon_it;

        /// Positions
        TSeqPos product_start = exon.GetProduct_start().AsSeqPos();
        TSeqPos product_end = exon.GetProduct_end().AsSeqPos();
        if (product_start > product_end) {
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                   "CSpliced_seg::Validate(): product_start > product_end");
        }
        if (product_start < product_position_limits.GetFrom() || product_position_limits.GetTo() < product_end) {
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                   "CSpliced_seg::Validate(): illegal product position in regard to poly-a and/or product-length ");
        }
        if (exon.GetGenomic_end() < exon.GetGenomic_start()) {
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                   "CSpliced_seg::Validate(): genomic_start > genomic_end");
        }


        /// Ids
        if ( !(IsSetProduct_id()  ||  exon.IsSetProduct_id()) ) {
             NCBI_THROW(CSeqalignException, eInvalidAlignment,
                        "product-id not set.");
        }
        if (IsSetProduct_id()  ==  exon.IsSetProduct_id()) {
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                       "product-id should be set on the level of Spliced-seg XOR Spliced-exon.");
        }
        if ( !(IsSetGenomic_id()  ||  exon.IsSetGenomic_id()) ) {
             NCBI_THROW(CSeqalignException, eInvalidAlignment,
                        "genomic-id not set.");
        }


        /// Strands
        if (IsSetProduct_strand()  &&  exon.IsSetProduct_strand()) {
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                       "product-strand can be set on level of Spliced-seg XOR Spliced-exon.");
        }
        bool product_plus = true;
        if (exon.CanGetProduct_strand()) {
            product_plus = exon.GetProduct_strand() != eNa_strand_minus;
        } else if (CanGetProduct_strand()) {
            product_plus = GetProduct_strand() != eNa_strand_minus;
        }
        if (prot  &&  !product_plus) {
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                       "Protein product cannot have a negative strand.");
        }


        /// Ranges
        if (exon.IsSetParts()) {
            TSeqPos exon_product_len = 0;
            TSeqPos exon_genomic_len = 0;
            ITERATE (CSpliced_exon::TParts, chunk_it, exon.GetParts()) {
                const CSpliced_exon_chunk& chunk = **chunk_it;
                
                TSeqPos chunk_product_len = 0;
                TSeqPos chunk_genomic_len = 0;
            
                switch (chunk.Which()) {
                case CSpliced_exon_chunk::e_Match: 
                    chunk_product_len = chunk_genomic_len = chunk.GetMatch();
                    break;
                case CSpliced_exon_chunk::e_Diag: 
                    chunk_product_len = chunk_genomic_len = chunk.GetDiag();
                    break;
                case CSpliced_exon_chunk::e_Mismatch:
                    chunk_product_len = chunk_genomic_len = chunk.GetMismatch();
                    break;
                case CSpliced_exon_chunk::e_Product_ins:
                    chunk_product_len = chunk.GetProduct_ins();
                    break;
                case CSpliced_exon_chunk::e_Genomic_ins:
                    chunk_genomic_len = chunk.GetGenomic_ins();
                    break;
                default:
                    break;
                }
                exon_product_len += chunk_product_len;
                exon_genomic_len += chunk_genomic_len;
            }
            if (exon_product_len != product_end - product_start + 1) {
                NCBI_THROW(CSeqalignException, eInvalidAlignment,
                           "Product exon range length is not consistent with exon chunks.");
            }
            if (exon_genomic_len != 
                exon.GetGenomic_end() - exon.GetGenomic_start() + 1) {
                NCBI_THROW(CSeqalignException, eInvalidAlignment,
                           "Genomic exon range length is not consistent with exon chunks.");
            }
        } else {
            TSeqPos exon_product_len = product_end - product_start + 1;
            TSeqPos exon_genomic_len = exon.GetGenomic_end() - exon.GetGenomic_start() + 1;
            if (exon_product_len != exon_genomic_len) {
                NCBI_THROW(CSeqalignException, eInvalidAlignment,
                           "Product and genomic exon range lengths are not consistent.");
            }
        }
    }

}


CRange<TSeqPos>
CSpliced_seg::GetSeqRange(TDim row) const
{
    if (GetExons().empty()) {
        NCBI_THROW(CSeqalignException, eInvalidAlignment,
                   "CSpliced_seg::GetSeqRange(): Spiced-seg is empty (has no exons)");
    }
    CRange<TSeqPos> result;
    switch (row) {
    case 0:
        switch (GetProduct_type()) {
        case eProduct_type_transcript:
            ITERATE(TExons, exon_it, GetExons()) {
                result.CombineWith
                    (CRange<TSeqPos>
                     ((*exon_it)->GetProduct_start().GetNucpos(),
                      (*exon_it)->GetProduct_end().GetNucpos()));
            }
            break;
        case eProduct_type_protein:
            ITERATE(TExons, exon_it, GetExons()) {
                result.CombineWith
                    (CRange<TSeqPos>
                     ((*exon_it)->GetProduct_start().GetProtpos().GetAmin(),
                      (*exon_it)->GetProduct_end().GetProtpos().GetAmin()));

            }
            break;
        default:
            NCBI_THROW(CSeqalignException, eInvalidAlignment,
                       "Invalid product type");
        }
        break;
    case 1:
        ITERATE(TExons, exon_it, GetExons()) {
            result.CombineWith
                (CRange<TSeqPos>
                 ((*exon_it)->GetGenomic_start(),
                  (*exon_it)->GetGenomic_end()));
        }
        break;
    default:
        NCBI_THROW(CSeqalignException, eInvalidRowNumber,
                   "CSpliced_seg::GetSeqRange(): Invalid row number");
    }
    return result;
}


TSeqPos
CSpliced_seg::GetSeqStart(TDim row) const
{
    const CSpliced_exon &first_exon = *(GetSeqStrand(row) == eNa_strand_minus
        ? GetExons().back() : GetExons().front());
    return first_exon.GetRowSeq_range(row, false).GetFrom();
}


TSeqPos
CSpliced_seg::GetSeqStop (TDim row) const
{
    const CSpliced_exon &last_exon = *(GetSeqStrand(row) == eNa_strand_minus
        ? GetExons().front() : GetExons().back());
    return last_exon.GetRowSeq_range(row, false).GetTo();
}


//////////////////////////////////////////////////////////////////////////////


static vector<TSignedSeqPos>
s_CalculateStarts(const vector<TSeqPos>& lens, ENa_strand strand,
                  TSeqPos start, TSeqPos end)
{
    vector<TSignedSeqPos> rv;
    rv.reserve(lens.size());
    TSignedSeqPos offset = 0;
    ITERATE (vector<TSeqPos>, len, lens) {
        if (*len == 0) {
            // a gap
            rv.push_back(-1);
        } else {
            if (IsReverse(strand)) {
                offset += *len;
                rv.push_back((end + 1) - offset);
            } else {
                rv.push_back(start + offset);
                offset += *len;
            }
        }
    }
    return rv;
}


static CRef<CDense_seg>
s_ExonToDenseg(const CSpliced_exon& exon,
               ENa_strand product_strand, ENa_strand genomic_strand,
               const CSeq_id& product_id, const CSeq_id& genomic_id)
{
    CRef<CDense_seg> ds(new CDense_seg);

    vector<TSeqPos> product_lens;
    vector<TSeqPos> genomic_lens;
    if (exon.IsSetParts()  &&  !exon.GetParts().empty()) {
        ITERATE (CSpliced_exon::TParts, iter, exon.GetParts()) {
            const CSpliced_exon_chunk& part = **iter;
            if (part.IsMatch()) {
                product_lens.push_back(part.GetMatch());
                genomic_lens.push_back(part.GetMatch());
            } else if (part.IsMismatch()) {
                product_lens.push_back(part.GetMismatch());
                genomic_lens.push_back(part.GetMismatch());
            } else if (part.IsDiag()) {
                product_lens.push_back(part.GetDiag());
                genomic_lens.push_back(part.GetDiag());
            } else if (part.IsProduct_ins()) {
                product_lens.push_back(part.GetProduct_ins());
                genomic_lens.push_back(0);
            } else if (part.IsGenomic_ins()) {
                product_lens.push_back(0);
                genomic_lens.push_back(part.GetGenomic_ins());
            } else {
                throw runtime_error("unhandled part type in Spliced-enon");
            }
        }
    }
    else {
        TSeqPos len = exon.GetGenomic_end() - exon.GetGenomic_start() + 1;
        genomic_lens.push_back(len);
        product_lens.push_back(len);
    }

    CDense_seg::TLens& lens = ds->SetLens();
    lens.reserve(product_lens.size());
    for (unsigned int i = 0; i < product_lens.size(); ++i) {
        lens.push_back(max(product_lens[i], genomic_lens[i]));
    }

    if (exon.IsSetProduct_strand()) {
        product_strand = exon.GetProduct_strand();
    }
    if (exon.IsSetGenomic_strand()) {
        genomic_strand = exon.GetGenomic_strand();
    }

    vector<TSignedSeqPos> product_starts;
    product_starts =
        s_CalculateStarts(product_lens, product_strand,
                          exon.GetProduct_start().AsSeqPos(),
                          exon.GetProduct_end().AsSeqPos());
    vector<TSignedSeqPos> genomic_starts =
        s_CalculateStarts(genomic_lens, genomic_strand,
                          exon.GetGenomic_start(),
                          exon.GetGenomic_end());

    CDense_seg::TStarts& starts = ds->SetStarts();
    starts.reserve(product_starts.size() + genomic_starts.size());
    for (unsigned int i = 0; i < lens.size(); ++i) {
        starts.push_back(product_starts[i]);  // product row first
        starts.push_back(genomic_starts[i]);
    }
    ds->SetIds().push_back(CRef<CSeq_id>(SerialClone(
        exon.IsSetProduct_id() ? exon.GetProduct_id() : product_id)));
    ds->SetIds().push_back(CRef<CSeq_id>(SerialClone(
        exon.IsSetGenomic_id() ? exon.GetGenomic_id() : genomic_id)));

    // Set strands, unless they're both plus
    if (!(product_strand == eNa_strand_plus
          && genomic_strand == eNa_strand_plus)) {
        CDense_seg::TStrands& strands = ds->SetStrands();
        for (unsigned int i = 0; i < lens.size(); ++i) {
            strands.push_back(product_strand);
            strands.push_back(genomic_strand);
        }
    }

    ds->SetNumseg(lens.size());

    ds->Compact();  // join adjacent match/mismatch/diag parts

    /// copy scores
    if (exon.IsSetScores()) {
        ITERATE (CSpliced_exon::TScores::Tdata, iter, exon.GetScores().Get()) {
            CRef<CScore> s(new CScore);
            s->Assign(**iter);
            ds->SetScores().push_back(s);
        }
    }

    return ds;
}


CRef<CSeq_align> CSpliced_seg::AsDiscSeg() const
{
    CRef<CSeq_align> disc(new CSeq_align);
    disc->SetType(CSeq_align::eType_disc);

    switch (GetProduct_type()) {
    case eProduct_type_transcript:
        {{
             ENa_strand product_strand = eNa_strand_plus;
             if (IsSetProduct_strand()) {
                 product_strand = GetProduct_strand();
             }
             ENa_strand genomic_strand = eNa_strand_plus;
             if (IsSetGenomic_strand()) {
                 genomic_strand = GetGenomic_strand();
             }
             const CSeq_id& product_id = GetProduct_id();
             const CSeq_id& genomic_id = GetGenomic_id();

             //for exon in spliced_seg.GetSegs().GetSpliced().GetExons()[:] {
             ITERATE (CSpliced_seg::TExons, exon, GetExons()) {
                 CRef<CDense_seg> ds = s_ExonToDenseg(**exon,
                                                      product_strand, genomic_strand,
                                                      product_id, genomic_id);
                 CRef<CSeq_align> ds_align(new CSeq_align);
                 ds_align->SetSegs().SetDenseg(*ds);
                 ds_align->SetType(CSeq_align::eType_partial);
                 disc->SetSegs().SetDisc().Set().push_back(ds_align);
             }
         }}
        break;

    case eProduct_type_protein:
        {{
             ENa_strand product_strand = eNa_strand_plus;
             ENa_strand genomic_strand = eNa_strand_plus;
             if (IsSetGenomic_strand()) {
                 genomic_strand = GetGenomic_strand();
             }
             const CSeq_id& product_id = GetProduct_id();
             const CSeq_id& genomic_id = GetGenomic_id();

             //for exon in spliced_seg.GetSegs().GetSpliced().GetExons()[:] {
             ITERATE (CSpliced_seg::TExons, exon, GetExons()) {
                 CRef<CDense_seg> ds = s_ExonToDenseg(**exon,
                                                      product_strand, genomic_strand,
                                                      product_id, genomic_id);
                 ds->SetWidths().push_back(3);
                 ds->SetWidths().push_back(1);
                 CRef<CSeq_align> ds_align(new CSeq_align);
                 ds_align->SetSegs().SetDenseg(*ds);
                 ds_align->SetType(CSeq_align::eType_partial);
                 disc->SetSegs().SetDisc().Set().push_back(ds_align);
             }
         }}
        break;

    default:
        NCBI_THROW(CException, eUnknown,
                   "unhandled product type in spliced seg");
        break;
    }

    return disc;
}


END_objects_SCOPE // namespace ncbi::objects::

END_NCBI_SCOPE

/* Original file checksum: lines: 57, chars: 1732, CRC32: fa335290 */
