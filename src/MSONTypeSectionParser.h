//
//  MSONTypeSectionParser.h
//  snowcrash
//
//  Created by Pavan Kumar Sunkara on 11/4/14.
//  Copyright (c) 2014 Apiary Inc. All rights reserved.
//

#ifndef SNOWCRASH_MSONTYPESECTIONPARSER_H
#define SNOWCRASH_MSONTYPESECTIONPARSER_H

#include "SectionParser.h"
#include "MSONUtility.h"

using namespace scpl;

namespace snowcrash {

    /** MSON Default Type Section matching regex */
    const char* const MSONDefaultTypeSectionRegex = "^[[:blank:]]*[Dd]efault[[:blank:]]*(:.*)?$";

    /** MSON Sample Type Section matching regex */
    const char* const MSONSampleTypeSectionRegex = "^[[:blank:]]*[Ss]ample[[:blank:]]*(:.*)?$";

    /** MSON Value Members Type Section matching regex */
    const char* const MSONValueMembersTypeSectionRegex = "^[[:blank:]]*([Ii]tems|[Mm]embers)[[:blank:]]*$";

    /** MSON Property Members Type Section matching regex */
    const char* const MSONPropertyMembersTypeSectionRegex = "^[[:blank:]]*([Pp]roperties)[[:blank:]]*$";

    /**
     * MSON Type Section Section Processor
     */
    template<>
    struct SectionProcessor<mson::TypeSection> : public SignatureSectionProcessorBase<mson::TypeSection> {

        static SignatureTraits signatureTraits() {

            SignatureTraits signatureTraits(SignatureTraits::IdentifierTrait |
                                            SignatureTraits::ValuesTrait);

            return signatureTraits;
        }

        static MarkdownNodeIterator finalizeSignature(const MarkdownNodeIterator& node,
                                                      SectionParserData& pd,
                                                      const Signature& signature,
                                                      const ParseResultRef<mson::TypeSection>& out) {

            bool assignValues = false;

            if (IEqual<std::string>()(signature.identifier, "Default")) {

                out.node.type = mson::DefaultTypeSectionType;
                assignValues = true;
            }
            else if (IEqual<std::string>()(signature.identifier, "Sample")) {

                out.node.type = mson::SampleTypeSectionType;
                assignValues = true;
            }
            else if (IEqual<std::string>()(signature.identifier, "Items") ||
                     IEqual<std::string>()(signature.identifier, "Members")) {

                if (out.node.baseType != mson::ValueBaseType) {

                    //WARN: Items/Members should only be allowed for value types
                    std::stringstream ss;

                    ss << "type section `" << signature.identifier;
                    ss << "` not allowed for a type sub-typed from a primitive or object type";

                    mdp::CharactersRangeSet sourceMap = mdp::BytesRangeSetToCharactersRangeSet(node->sourceMap, pd.sourceData);
                    out.report.warnings.push_back(Warning(ss.str(),
                                                          LogicalErrorWarning,
                                                          sourceMap));

                    return node;
                }

                out.node.type = mson::MemberTypeSectionType;
            }
            else if (IEqual<std::string>()(signature.identifier, "Properties")) {

                if (out.node.baseType != mson::PropertyBaseType &&
                    out.node.baseType != mson::ImplicitPropertyBaseType) {

                    //WARN: Properties should only be allowed for object types
                    std::stringstream ss;

                    ss << "type section `" << signature.identifier;
                    ss << "` is only allowed for a type sub-typed from an object type";

                    mdp::CharactersRangeSet sourceMap = mdp::BytesRangeSetToCharactersRangeSet(node->sourceMap, pd.sourceData);
                    out.report.warnings.push_back(Warning(ss.str(),
                                                          LogicalErrorWarning,
                                                          sourceMap));

                    return node;
                }

                out.node.type = mson::MemberTypeSectionType;
            }

            if (assignValues &&
                (!signature.values.empty() || !signature.value.empty())) {

                if (out.node.baseType == mson::PrimitiveBaseType ||
                    out.node.baseType == mson::ImplicitPrimitiveBaseType) {

                    out.node.content.value = signature.value;
                }
                else if (out.node.baseType == mson::ValueBaseType) {

                    for (size_t i = 0; i < signature.values.size(); i++) {

                        mson::MemberType memberType;
                        memberType.build(mson::parseValue(signature.values[i]));

                        out.node.content.members().push_back(memberType);
                    }
                }
                else if (out.node.baseType == mson::PropertyBaseType ||
                         out.node.baseType == mson::ImplicitPropertyBaseType) {

                    // WARN: sample/default is for an object but it has values in signature
                    mdp::CharactersRangeSet sourceMap = mdp::BytesRangeSetToCharactersRangeSet(node->sourceMap, pd.sourceData);
                    out.report.warnings.push_back(Warning("a sample and/or default type section for a type which is sub-typed from an object cannot have value(s) beside the keyword",
                                                          LogicalErrorWarning,
                                                          sourceMap));
                }
            }

            if (assignValues && !signature.remainingContent.empty() &&
                (out.node.baseType == mson::PrimitiveBaseType ||
                 out.node.baseType == mson::ImplicitPrimitiveBaseType)) {

                out.node.content.value += signature.remainingContent;
            }

            return ++MarkdownNodeIterator(node);
        }

        NO_SECTION_DESCRIPTION(mson::TypeSection)

        static MarkdownNodeIterator processNestedSection(const MarkdownNodeIterator&,
                                                         const MarkdownNodes&,
                                                         SectionParserData&,
                                                         const ParseResultRef<mson::TypeSection>&);

        static SectionType sectionType(const MarkdownNodeIterator& node) {

            mdp::ByteBuffer subject, remaining;

            if (node->type == mdp::HeaderMarkdownNodeType &&
                !node->text.empty()) {

                subject = node->text;
            }
            else if (node->type == mdp::ListItemMarkdownNodeType &&
                     !node->children().empty()) {

                subject = node->children().front().text;
            }

            subject = GetFirstLine(subject, remaining);
            TrimString(subject);

            if (RegexMatch(subject, MSONDefaultTypeSectionRegex) ||
                RegexMatch(subject, MSONSampleTypeSectionRegex)) {

                return MSONSampleDefaultSectionType;
            }

            if (RegexMatch(subject, MSONValueMembersTypeSectionRegex)) {
                return MSONValueMembersSectionType;
            }

            if (RegexMatch(subject, MSONPropertyMembersTypeSectionRegex)) {
                return MSONPropertyMembersSectionType;
            }

            return UndefinedSectionType;
        }

        static SectionType nestedSectionType(const MarkdownNodeIterator&);
    };

    /** MSON Type Section (Header) Section Parser */
    typedef SectionParser<mson::TypeSection, HeaderSectionAdapter> MSONTypeSectionHeaderParser;

    /** MSON Type Section (List) Section Parser */
    typedef SectionParser<mson::TypeSection, ListSectionAdapter> MSONTypeSectionListParser;
}

#endif
