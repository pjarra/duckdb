//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/parser/tableref/joinref.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/enums/join_type.hpp"
#include "duckdb/common/enums/joinref_type.hpp"
#include "duckdb/common/unordered_set.hpp"
#include "duckdb/parser/parsed_expression.hpp"
#include "duckdb/parser/tableref.hpp"
#include "duckdb/common/vector.hpp"

namespace duckdb {

//! Represents a JOIN between two expressions
class JoinRef : public TableRef {
public:
	explicit JoinRef(JoinRefType ref_type)
	    : TableRef(TableReferenceType::JOIN), type(JoinType::INNER), ref_type(ref_type) {
	}

	//! The left hand side of the join
	unique_ptr<TableRef> left;
	//! The right hand side of the join
	unique_ptr<TableRef> right;
	//! The join condition
	unique_ptr<ParsedExpression> condition;
	//! The join type
	JoinType type;
	//! Join condition type
	JoinRefType ref_type;
	//! The set of USING columns (if any)
	vector<string> using_columns;

public:
	string ToString() const override;
	bool Equals(const TableRef *other_p) const override;

	unique_ptr<TableRef> Copy() override;

	//! Serializes a blob into a JoinRef
	void Serialize(FieldWriter &serializer) const override;
	//! Deserializes a blob back into a JoinRef
	static unique_ptr<TableRef> Deserialize(FieldReader &source);

	void FormatSerialize(FormatSerializer &serializer) const override;
	static unique_ptr<TableRef> FormatDeserialize(FormatDeserializer &source);
};
} // namespace duckdb
