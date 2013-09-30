#ifndef AST_H
#define AST_H

#include <vector>

#include "Types.h"

namespace FPTL {
namespace Parser {

	class TTuple;

	//
	// ������� ������� ������ � ��� �������� ������ ��������� ������ � ������������ ������.
	//
	class ASTNode
	{
	public:

		// �������� ����� ������������ ��������������� ������.
		enum ASTNodeType
		{
			Unknown = 0,

			FunctionalProgramDef,

			DataTypeDefinitionsBlocks,
			DataTypeDefinitionBlock,
			TypeParametersList,
			ConstructorsDefinitionList,
			Constructor,
			ConstructorParametersList,

			TypeExpression,
			TypeExpressionsList,
			TypesDefinitionList,
			DefinitionsList,

			// DefinitionNode
			TypeParameterDefinition,
			TypeDefinition,
			TypeConstructorDefinition,
			Definition,
			InputVarDefinition,
			FunctionParameterDefinition,
		
			VariantTerm,
			ConditionTerm,
			CompositionTerm,
			SequentialTerm,
			FormalParametersList,
			FunctionBlock,
			FuncArgumentsList,

			// NameRefNode
			BaseType,
			Template,
			Type,               // TypeName
			ConstructorName,
			DestructorName,
			BuildInFunction,
			FuncObjectName,
			TypeParamName,
			FuncObjectWithParameters, // ParametrizedCall
			InputVarName,
			FuncParameterName,
			RunningSchemeName,

			Application,
			InputVarDefinitionList,
			InputVarList,
			InputVar,
			
			// ConstantNode
			TupleElemNumber,
			DoubleConstant,
			FloatConstant,
			StringConstant,
			IntConstant,
			LongLongConstant,
			TrueValue,
			FalseValue,

			// ExpressionNode
			ValueComposition,
			ValueConstructor
		};

		ASTNode( ASTNodeType aType );
		virtual ~ASTNode();

		ASTNodeType getType() const { return mType; }

		virtual void accept( class NodeVisitor * aVisitor ) = 0;

		virtual ASTNode * copy() const = 0;

		// ���������� ���������� ������, � ������� ������� ��� �������.
		int cyclicIndex() const;

		void increaseCyclicIndex();

		bool isRecursive() const { return mCyclicIndex > 0; }

		// ��������� ���� ������ ����� ����� ���������, �������� ������� � ���� ������.
		// ������ ����� �������� �������� ����� ����������.
		virtual int numParameters() const { return 0; }

		// ���������� � ���� ����.
		TTuple getTypeTuple() const;
		void setTypeTuple(const TTuple & aTuple);

	private:

		ASTNode( const ASTNode & );
		ASTNode & operator=( const ASTNode & );

		ASTNodeType mType;
		int mCyclicIndex;
		TTuple mTypeTuple;
	};

} // Parser
} // FPTL

#endif