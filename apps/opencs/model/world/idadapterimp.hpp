#ifndef CSM_WOLRD_IDADAPTERIMP_H
#define CSM_WOLRD_IDADAPTERIMP_H

#include <QVariant>

#include <components/esm/loadpgrd.hpp>
#include <components/esm/loadregn.hpp>
#include <components/esm/loadfact.hpp>
#include <components/esm/effectlist.hpp>
#include <components/esm/loadmgef.hpp> // for converting magic effect id to string & back
#include <components/esm/loadskil.hpp> // for converting skill names
#include <components/esm/attr.hpp>     // for converting attributes

#include "idadapter.hpp"
#include "nestedtablewrapper.hpp"

namespace CSMWorld
{
    template<typename ESXRecordT>
    class PathgridPointListAdapter : public NestedIdAdapter<ESXRecordT>
    {
    public:
        PathgridPointListAdapter () {}

        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT pathgrid = record.get();

            ESM::Pathgrid::PointList& points = pathgrid.mPoints;

            // blank row
            ESM::Pathgrid::Point point;
            point.mX = 0;
            point.mY = 0;
            point.mZ = 0;
            point.mAutogenerated = 0;
            point.mConnectionNum = 0;
            point.mUnknown = 0;

            // inserting a point should trigger re-indexing of the edges
            //
            // FIXME: undo does not restore edges table view
            // FIXME: does not auto refresh edges table view
            std::vector<ESM::Pathgrid::Edge>::iterator iter = pathgrid.mEdges.begin();
            for (;iter != pathgrid.mEdges.end(); ++iter)
            {
                if ((*iter).mV0 >= position)
                    (*iter).mV0++;
                if ((*iter).mV1 >= position)
                    (*iter).mV1++;
            }

            points.insert(points.begin()+position, point);
            pathgrid.mData.mS2 += 1; // increment the number of points

            record.setModified (pathgrid);
        }

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT pathgrid = record.get();

            ESM::Pathgrid::PointList& points = pathgrid.mPoints;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (points.size()))
                throw std::runtime_error ("index out of range");

            // deleting a point should trigger re-indexing of the edges
            // dangling edges are not allowed and hence removed
            //
            // FIXME: undo does not restore edges table view
            // FIXME: does not auto refresh edges table view
            std::vector<ESM::Pathgrid::Edge>::iterator iter = pathgrid.mEdges.begin();
            for (; iter != pathgrid.mEdges.end();)
            {
                if (((*iter).mV0 == rowToRemove) || ((*iter).mV1 == rowToRemove))
                    iter = pathgrid.mEdges.erase(iter);
                else
                {
                    if ((*iter).mV0 > rowToRemove)
                        (*iter).mV0--;

                    if ((*iter).mV1 > rowToRemove)
                        (*iter).mV1--;

                    ++iter;
                }
            }
            points.erase(points.begin()+rowToRemove);
            pathgrid.mData.mS2 -= 1; // decrement the number of points

            record.setModified (pathgrid);
        }

        struct PathgridPointsWrap : public NestedTableWrapperBase
        {
            ESM::Pathgrid mRecord;

            PathgridPointsWrap(ESM::Pathgrid pathgrid)
                : mRecord(pathgrid) {}

            virtual ~PathgridPointsWrap() {}

            virtual int size() const
            {
                return mRecord.mPoints.size(); // used in IdTree::setNestedTable()
            }
        };

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            record.get().mPoints =
                static_cast<const PathgridPointsWrap &>(nestedTable).mRecord.mPoints;
            record.get().mData.mS2 =
                static_cast<const PathgridPointsWrap &>(nestedTable).mRecord.mData.mS2;
            // also update edges in case points were added/removed
            record.get().mEdges =
                static_cast<const PathgridPointsWrap &>(nestedTable).mRecord.mEdges;
        }

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new PathgridPointsWrap(record.get());
        }

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
        {
            ESM::Pathgrid::Point point = record.get().mPoints[subRowIndex];
            switch (subColIndex)
            {
                case 0: return subRowIndex;
                case 1: return point.mX;
                case 2: return point.mY;
                case 3: return point.mZ;
                default: throw std::runtime_error("Pathgrid point subcolumn index out of range");
            }
        }

        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const
        {
            ESXRecordT pathgrid = record.get();
            ESM::Pathgrid::Point point = pathgrid.mPoints[subRowIndex];
            switch (subColIndex)
            {
                case 0: break;
                case 1: point.mX = value.toInt(); break;
                case 2: point.mY = value.toInt(); break;
                case 3: point.mZ = value.toInt(); break;
                default: throw std::runtime_error("Pathgrid point subcolumn index out of range");
            }

            pathgrid.mPoints[subRowIndex] = point;

            record.setModified (pathgrid);
        }

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 4;
        }

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mPoints.size());
        }
    };

    template<typename ESXRecordT>
    class PathgridEdgeListAdapter : public NestedIdAdapter<ESXRecordT>
    {
    public:
        PathgridEdgeListAdapter () {}

        // FIXME: seems to be auto-sorted in the dialog table display after insertion
        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT pathgrid = record.get();

            ESM::Pathgrid::EdgeList& edges = pathgrid.mEdges;

            // blank row
            ESM::Pathgrid::Edge edge;
            edge.mV0 = 0;
            edge.mV1 = 0;

            // NOTE: inserting a blank edge does not really make sense, perhaps this should be a
            // logic_error exception
            //
            // Currently the code assumes that the end user to know what he/she is doing.
            // e.g. Edges come in pairs, from points a->b and b->a
            edges.insert(edges.begin()+position, edge);

            record.setModified (pathgrid);
        }

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT pathgrid = record.get();

            ESM::Pathgrid::EdgeList& edges = pathgrid.mEdges;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (edges.size()))
                throw std::runtime_error ("index out of range");

            edges.erase(edges.begin()+rowToRemove);

            record.setModified (pathgrid);
        }

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            record.get().mEdges =
                static_cast<const NestedTableWrapper<ESM::Pathgrid::EdgeList> &>(nestedTable).mNestedTable;
        }

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<ESM::Pathgrid::EdgeList>(record.get().mEdges);
        }

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
        {
            ESXRecordT pathgrid = record.get();

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (pathgrid.mEdges.size()))
                throw std::runtime_error ("index out of range");

            ESM::Pathgrid::Edge edge = pathgrid.mEdges[subRowIndex];
            switch (subColIndex)
            {
                case 0: return subRowIndex;
                case 1: return edge.mV0;
                case 2: return edge.mV1;
                default: throw std::runtime_error("Pathgrid edge subcolumn index out of range");
            }
        }

        // FIXME: detect duplicates in mEdges
        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const
        {
            ESXRecordT pathgrid = record.get();

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (pathgrid.mEdges.size()))
                throw std::runtime_error ("index out of range");

            ESM::Pathgrid::Edge edge = pathgrid.mEdges[subRowIndex];
            switch (subColIndex)
            {
                case 0: break;
                case 1: edge.mV0 = value.toInt(); break;
                case 2: edge.mV1 = value.toInt(); break;
                default: throw std::runtime_error("Pathgrid edge subcolumn index out of range");
            }

            pathgrid.mEdges[subRowIndex] = edge;

            record.setModified (pathgrid);
        }

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 3;
        }

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mEdges.size());
        }
    };

    template<typename ESXRecordT>
    class FactionReactionsAdapter : public NestedIdAdapter<ESXRecordT>
    {
    public:
        FactionReactionsAdapter () {}

        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT faction = record.get();

            std::map<std::string, int>& reactions = faction.mReactions;

            // blank row
            reactions.insert(std::make_pair("", 0));

            record.setModified (faction);
        }

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT faction = record.get();

            std::map<std::string, int>& reactions = faction.mReactions;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (reactions.size()))
                throw std::runtime_error ("index out of range");

            // FIXME: how to ensure that the map entries correspond to table indicies?
            // WARNING: Assumed that the table view has the same order as std::map
            std::map<std::string, int>::iterator iter = reactions.begin();
            for(int i = 0; i < rowToRemove; ++i)
                iter++;
            reactions.erase(iter);

            record.setModified (faction);
        }

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            record.get().mReactions =
                static_cast<const NestedTableWrapper<std::map<std::string, int> >&>(nestedTable).mNestedTable;
        }

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::map<std::string, int> >(record.get().mReactions);
        }

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
        {
            ESXRecordT faction = record.get();

            std::map<std::string, int>& reactions = faction.mReactions;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (reactions.size()))
                throw std::runtime_error ("index out of range");

            // FIXME: how to ensure that the map entries correspond to table indicies?
            // WARNING: Assumed that the table view has the same order as std::map
            std::map<std::string, int>::const_iterator iter = reactions.begin();
            for(int i = 0; i < subRowIndex; ++i)
                iter++;
            switch (subColIndex)
            {
                case 0: return QString((*iter).first.c_str());
                case 1: return (*iter).second;
                default: throw std::runtime_error("Faction reactions subcolumn index out of range");
            }
        }

        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const
        {
            ESXRecordT faction = record.get();

            std::map<std::string, int>& reactions = faction.mReactions;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (reactions.size()))
                throw std::runtime_error ("index out of range");

            // FIXME: how to ensure that the map entries correspond to table indicies?
            // WARNING: Assumed that the table view has the same order as std::map
            std::map<std::string, int>::iterator iter = reactions.begin();
            for(int i = 0; i < subRowIndex; ++i)
                iter++;

            std::string factionId = (*iter).first;
            int reaction = (*iter).second;

            switch (subColIndex)
            {
                case 0:
                {
                    reactions.erase(iter);
                    reactions.insert(std::make_pair(value.toString().toUtf8().constData(), reaction));
                    break;
                }
                case 1:
                {
                    reactions[factionId] = value.toInt();
                    break;
                }
                default: throw std::runtime_error("Faction reactions subcolumn index out of range");
            }

            record.setModified (faction);
        }

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 2;
        }

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mReactions.size());
        }
    };

    template<typename ESXRecordT>
    class RegionSoundListAdapter : public NestedIdAdapter<ESXRecordT>
    {
    public:
        RegionSoundListAdapter () {}

        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT region = record.get();

            std::vector<typename ESXRecordT::SoundRef>& soundList = region.mSoundList;

            // blank row
            typename ESXRecordT::SoundRef soundRef;
            soundRef.mSound.assign("");
            soundRef.mChance = 0;

            soundList.insert(soundList.begin()+position, soundRef);

            record.setModified (region);
        }

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT region = record.get();

            std::vector<typename ESXRecordT::SoundRef>& soundList = region.mSoundList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (soundList.size()))
                throw std::runtime_error ("index out of range");

            soundList.erase(soundList.begin()+rowToRemove);

            record.setModified (region);
        }

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            record.get().mSoundList =
                static_cast<const NestedTableWrapper<std::vector<typename ESXRecordT::SoundRef> >&>(nestedTable).mNestedTable;
        }

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<typename ESXRecordT::SoundRef> >(record.get().mSoundList);
        }

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
        {
            ESXRecordT region = record.get();

            std::vector<typename ESXRecordT::SoundRef>& soundList = region.mSoundList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (soundList.size()))
                throw std::runtime_error ("index out of range");

            typename ESXRecordT::SoundRef soundRef = soundList[subRowIndex];
            switch (subColIndex)
            {
                case 0: return QString(soundRef.mSound.toString().c_str());
                case 1: return soundRef.mChance;
                default: throw std::runtime_error("Region sounds subcolumn index out of range");
            }
        }

        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const
        {
            ESXRecordT region = record.get();

            std::vector<typename ESXRecordT::SoundRef>& soundList = region.mSoundList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (soundList.size()))
                throw std::runtime_error ("index out of range");

            typename ESXRecordT::SoundRef soundRef = soundList[subRowIndex];
            switch (subColIndex)
            {
                case 0: soundRef.mSound.assign(value.toString().toUtf8().constData()); break;
                case 1: soundRef.mChance = static_cast<unsigned char>(value.toInt()); break;
                default: throw std::runtime_error("Region sounds subcolumn index out of range");
            }

            region.mSoundList[subRowIndex] = soundRef;

            record.setModified (region);
        }

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 2;
        }

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mSoundList.size());
        }
    };

    template<typename ESXRecordT>
    class SpellListAdapter : public NestedIdAdapter<ESXRecordT>
    {
    public:
        SpellListAdapter () {}

        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            // blank row
            std::string spell = "";

            spells.insert(spells.begin()+position, spell);

            record.setModified (raceOrBthSgn);
        }

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (spells.size()))
                throw std::runtime_error ("index out of range");

            spells.erase(spells.begin()+rowToRemove);

            record.setModified (raceOrBthSgn);
        }

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            record.get().mPowers.mList =
                static_cast<const NestedTableWrapper<std::vector<std::string> >&>(nestedTable).mNestedTable;
        }

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<std::string> >(record.get().mPowers.mList);
        }

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (spells.size()))
                throw std::runtime_error ("index out of range");

            std::string spell = spells[subRowIndex];
            switch (subColIndex)
            {
                case 0: return QString(spell.c_str());
                default: throw std::runtime_error("Spells subcolumn index out of range");
            }
        }

        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const
        {
            ESXRecordT raceOrBthSgn = record.get();

            std::vector<std::string>& spells = raceOrBthSgn.mPowers.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (spells.size()))
                throw std::runtime_error ("index out of range");

            std::string spell = spells[subRowIndex];
            switch (subColIndex)
            {
                case 0: spell = value.toString().toUtf8().constData(); break;
                default: throw std::runtime_error("Spells subcolumn index out of range");
            }

            raceOrBthSgn.mPowers.mList[subRowIndex] = spell;

            record.setModified (raceOrBthSgn);
        }

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 1;
        }

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mPowers.mList.size());
        }
    };

    template<typename ESXRecordT>
    class EffectsListAdapter : public NestedIdAdapter<ESXRecordT>
    {
    public:
        EffectsListAdapter () {}

        virtual void addNestedRow(Record<ESXRecordT>& record, int position) const
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            // blank row
            ESM::ENAMstruct effect;
            effect.mEffectID = 0;
            effect.mSkill = 0;
            effect.mAttribute = 0;
            effect.mRange = 0;
            effect.mArea = 0;
            effect.mDuration = 0;
            effect.mMagnMin = 0;
            effect.mMagnMax = 0;

            effectsList.insert(effectsList.begin()+position, effect);

            record.setModified (magic);
        }

        virtual void removeNestedRow(Record<ESXRecordT>& record, int rowToRemove) const
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            if (rowToRemove < 0 || rowToRemove >= static_cast<int> (effectsList.size()))
                throw std::runtime_error ("index out of range");

            effectsList.erase(effectsList.begin()+rowToRemove);

            record.setModified (magic);
        }

        virtual void setNestedTable(Record<ESXRecordT>& record, const NestedTableWrapperBase& nestedTable) const
        {
            record.get().mEffects.mList =
                static_cast<const NestedTableWrapper<std::vector<ESM::ENAMstruct> >&>(nestedTable).mNestedTable;
        }

        virtual NestedTableWrapperBase* nestedTable(const Record<ESXRecordT>& record) const
        {
            // deleted by dtor of NestedTableStoring
            return new NestedTableWrapper<std::vector<ESM::ENAMstruct> >(record.get().mEffects.mList);
        }

        virtual QVariant getNestedData(const Record<ESXRecordT>& record, int subRowIndex, int subColIndex) const
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (effectsList.size()))
                throw std::runtime_error ("index out of range");

            ESM::ENAMstruct effect = effectsList[subRowIndex];
            switch (subColIndex)
            {
                case 0:
                {
                    // indexToId() prepends "#d+" hence not so user friendly
                    QString effectId(ESM::MagicEffect::effectIdToString(effect.mEffectID).c_str());
                    return effectId.remove(0, 7); // 7 == sizeof("sEffect") - 1
                }
                case 1:
                {
                    switch (effect.mSkill)
                    {
                        // see ESM::Skill::SkillEnum in <component/esm/loadskil.hpp>
                        case ESM::Skill::Block:
                        case ESM::Skill::Armorer:
                        case ESM::Skill::MediumArmor:
                        case ESM::Skill::HeavyArmor:
                        case ESM::Skill::BluntWeapon:
                        case ESM::Skill::LongBlade:
                        case ESM::Skill::Axe:
                        case ESM::Skill::Spear:
                        case ESM::Skill::Athletics:
                        case ESM::Skill::Enchant:
                        case ESM::Skill::Destruction:
                        case ESM::Skill::Alteration:
                        case ESM::Skill::Illusion:
                        case ESM::Skill::Conjuration:
                        case ESM::Skill::Mysticism:
                        case ESM::Skill::Restoration:
                        case ESM::Skill::Alchemy:
                        case ESM::Skill::Unarmored:
                        case ESM::Skill::Security:
                        case ESM::Skill::Sneak:
                        case ESM::Skill::Acrobatics:
                        case ESM::Skill::LightArmor:
                        case ESM::Skill::ShortBlade:
                        case ESM::Skill::Marksman:
                        case ESM::Skill::Mercantile:
                        case ESM::Skill::Speechcraft:
                        case ESM::Skill::HandToHand:
                        {
                            return QString(ESM::Skill::sSkillNames[effect.mSkill].c_str());
                        }
                        case -1: return QString("N/A");
                        default: return QVariant();
                    }
                }
                case 2:
                {
                    switch (effect.mAttribute)
                    {
                        // see ESM::Attribute::AttributeID in <component/esm/attr.hpp>
                        case ESM::Attribute::Strength:
                        case ESM::Attribute::Intelligence:
                        case ESM::Attribute::Willpower:
                        case ESM::Attribute::Agility:
                        case ESM::Attribute::Speed:
                        case ESM::Attribute::Endurance:
                        case ESM::Attribute::Personality:
                        case ESM::Attribute::Luck:
                        {
                            return QString(ESM::Attribute::sAttributeNames[effect.mAttribute].c_str());
                        }
                        case -1: return QString("N/A");
                        default: return QVariant();
                    }
                }
                case 3:
                {
                    switch (effect.mRange)
                    {
                        // see ESM::RangeType in <component/esm/defs.hpp>
                        case ESM::RT_Self: return QString("Self");
                        case ESM::RT_Touch: return QString("Touch");
                        case ESM::RT_Target: return QString("Target");
                        default: return QVariant();
                    }
                }
                case 4: return effect.mArea;
                case 5: return effect.mDuration;
                case 6: return effect.mMagnMin;
                case 7: return effect.mMagnMax;
                default: throw std::runtime_error("Magic Effects subcolumn index out of range");
            }
        }

        virtual void setNestedData(Record<ESXRecordT>& record, const QVariant& value,
                                    int subRowIndex, int subColIndex) const
        {
            ESXRecordT magic = record.get();

            std::vector<ESM::ENAMstruct>& effectsList = magic.mEffects.mList;

            if (subRowIndex < 0 || subRowIndex >= static_cast<int> (effectsList.size()))
                throw std::runtime_error ("index out of range");

            ESM::ENAMstruct effect = effectsList[subRowIndex];
            switch (subColIndex)
            {
                case 0:
                {
                    effect.mEffectID =
                        ESM::MagicEffect::effectStringToId("sEffect"+value.toString().toStdString());
                    break;
                }
                case 1:
                {
                    std::string skillName = value.toString().toStdString();
                    if ("N/A" == skillName)
                    {
                        effect.mSkill = -1;
                        break;
                    }

                    for (unsigned int i = 0; i < ESM::Skill::Length; ++i)
                    {
                        if (ESM::Skill::sSkillNames[i] == skillName)
                        {
                            effect.mSkill = static_cast<signed char>(i);
                            break;
                        }
                    }
                    break;
                }
                case 2:
                {
                    std::string attr = value.toString().toStdString();
                    if ("N/A" == attr)
                    {
                        effect.mAttribute = -1;
                        break;
                    }

                    for (unsigned int i = 0; i < ESM::Attribute::Length; ++i)
                    {
                        if (ESM::Attribute::sAttributeNames[i] == attr)
                        {
                            effect.mAttribute = static_cast<signed char>(i);
                            break;
                        }
                    }
                    break;
                }
                case 3:
                {
                    std::string effectId = value.toString().toStdString();
                    if (effectId == "Self")
                        effect.mRange = ESM::RT_Self;
                    else if (effectId == "Touch")
                        effect.mRange = ESM::RT_Touch;
                    else if (effectId == "Target")
                        effect.mRange = ESM::RT_Target;
                    // else leave unchanged
                    break;
                }
                case 4: effect.mArea = value.toInt(); break;
                case 5: effect.mDuration = value.toInt(); break;
                case 6: effect.mMagnMin = value.toInt(); break;
                case 7: effect.mMagnMax = value.toInt(); break;
                default: throw std::runtime_error("Magic Effects subcolumn index out of range");
            }

            magic.mEffects.mList[subRowIndex] = effect;

            record.setModified (magic);
        }

        virtual int getNestedColumnsCount(const Record<ESXRecordT>& record) const
        {
            return 8;
        }

        virtual int getNestedRowsCount(const Record<ESXRecordT>& record) const
        {
            return static_cast<int>(record.get().mEffects.mList.size());
        }
    };
}

#endif // CSM_WOLRD_IDADAPTERIMP_H
