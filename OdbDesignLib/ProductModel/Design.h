#pragma once

#include "../odbdesign_export.h"
#include <string>
#include <memory>
#include <cstdint>
#include "Net.h"
#include "Component.h"
#include "Package.h"
#include "Part.h"
#include "design.pb.h"
#include "../IProtoBuffable.h"	
#include <map>
#include <vector>
#include "../FileModel/Design/StepDirectory.h"
#include "../FileModel/Design/EdaDataFile.h"
#include "../FileModel/Design/ComponentsFile.h"
#include "../FileModel/Design/FileArchive.h"


namespace Odb::Lib::ProductModel
{
	class ODBDESIGN_EXPORT Design : public IProtoBuffable<Protobuf::ProductModel::Design>
	{
	public:	
		Design();
		~Design();
		
		const std::string& GetName() const;
		const std::string& GetProductModel() const;
		
		const Net::Vector& GetNets() const;
		const Net::StringMap GetNetsByName() const;
		std::shared_ptr<Net> GetNet(const std::string& name) const;

		const Package::Vector& GetPackages() const;
		const Package::StringMap& GetPackagesByName() const;

		const Component::Vector& GetComponents() const;
		const Component::StringMap& GetComponentsByName() const;
		std::shared_ptr<Component> GetComponent(const std::string& refDes) const;

		const Part::Vector& GetParts() const;
		const Part::StringMap& GetPartsByName() const;
		
		std::shared_ptr<Net> GetNoneNet() const;

		bool Build(std::string path);
		bool Build(std::shared_ptr<FileModel::Design::FileArchive> pFileModel);	

		std::shared_ptr<FileModel::Design::FileArchive> GetFileModel() const;
		// Nulls the file model ON THIS INSTANCE. Only ever call this on a
		// privately-owned Design (a copy made for serving a clipped view) —
		// never on a cache-resident shared instance: the cached Design must
		// keep its file model for the gRPC GetDesign path and for response
		// pre-serialization (see DesignCache.h, M1.4).
		void ClipFileModel();

		// Inherited via IProtoBuffable
		//
		// The no-arg override produces the FULL wire flavor (normalized lists
		// included) because REST (to_json), to_pbstring, to_stream, and the
		// round-trip serialization tests all route through it and depend on the
		// complete payload. The GetDesign gRPC response is served by the
		// parameterized overload below: clients get the pruned flavor unless they
		// set GetDesignRequest.include_normalized_lists (the normalized
		// nets/components/packages/parts + ByName maps duplicate fileModel data
		// and dominate the gRPC payload).
		std::unique_ptr<Protobuf::ProductModel::Design> to_protobuf() const override;
		std::unique_ptr<Protobuf::ProductModel::Design> to_protobuf(bool includeNormalizedLists) const;
		void from_protobuf(const Protobuf::ProductModel::Design& message) override;

		// ---- M6 streaming support ----

		// Shared join state of the derived Connectivity view, computed once
		// and consumed by the FillConnectivity* section fillers below. Public
		// because the chunked GetDesignStream path builds the same sections
		// batch-by-batch and must emit byte-identical entries from the SAME
		// code (see FillConnectivityMessage).
		struct ConnectivityIndex
		{
			struct Edge
			{
				unsigned int net;
				unsigned int subnet;
			};
			std::map<std::string, std::map<unsigned int, Edge>> edgesByRefDes;
			std::map<unsigned int, std::map<std::string, std::vector<unsigned int>>> membersByNet;
			unsigned int noneNetOrdinal = 0;
			bool hasNoneNet = false;

			using MembersByNetConstIt = std::map<unsigned int, std::map<std::string, std::vector<unsigned int>>>::const_iterator;
		};

		// Builds the resolved-edge join state (identical to the local state
		// FillConnectivityMessage computes) plus the $NONE$ net ordinal.
		ConnectivityIndex BuildConnectivityIndex() const;

		// Section fillers: each appends ONE repeated section of the
		// Connectivity message in the same order FillConnectivityMessage
		// emits it. A caller may pass any (fresh, batch-sized) Connectivity
		// message; entries appended by these fillers are byte-identical to
		// the monolithic sections.
		void FillConnectivityNetEntries(Protobuf::ProductModel::Connectivity& connectivityMsg) const;
		void FillConnectivityPackagePins(Protobuf::ProductModel::Connectivity& connectivityMsg) const;
		void FillConnectivityPlacedComponents(const ConnectivityIndex& index, Protobuf::ProductModel::Connectivity& connectivityMsg) const;
		void FillConnectivityNetMembers(const ConnectivityIndex& index, Protobuf::ProductModel::Connectivity& connectivityMsg) const;

		// Single-entry emitters backing the section fillers above — the
		// granularity the chunked GetDesignStream path needs (one entry per
		// call, batched by the caller). AddConnectivityNetMemberEntry takes
		// an iterator into index.membersByNet.
		void AddConnectivityNetEntry(std::size_t netIndex, Protobuf::ProductModel::Connectivity& connectivityMsg) const;
		void AddConnectivityPackagePinEntry(std::size_t packageIndex, Protobuf::ProductModel::Connectivity& connectivityMsg) const;
		void AddConnectivityPlacedComponentEntry(const ConnectivityIndex& index, std::size_t componentIndex, Protobuf::ProductModel::Connectivity& connectivityMsg) const;
		void AddConnectivityNetMemberEntry(const ConnectivityIndex& index, ConnectivityIndex::MembersByNetConstIt netMembersIt, Protobuf::ProductModel::Connectivity& connectivityMsg) const;

		// Rough serialized-wire-size estimate from model counts (features,
		// component records, net/package records). O(number of layers/nets),
		// never O(records): it is the dispatch input for the monolithic-vs-
		// streaming serve decision and the OOM guard on monolithic GetDesign,
		// so it must stay cheap enough to run before any message is built.
		// includeNormalizedLists scales the estimate for the full wire flavor.
		std::uint64_t EstimateWireSizeBytes(bool includeNormalizedLists = false) const;

		typedef std::vector<std::shared_ptr<Design>> Vector;
		typedef std::map<std::string, std::shared_ptr<Design>> StringMap;

	private:
		std::string m_productModel;
		std::string m_name;

		std::shared_ptr<FileModel::Design::FileArchive> m_pFileModel;

		Net::Vector m_nets;
		Net::StringMap m_netsByName;

		Package::Vector m_packages;
		Package::StringMap m_packagesByName;

		Component::Vector m_components;
		Component::StringMap m_componentsByName;
		
		Part::Vector m_parts;
		Part::StringMap m_partsByName;

		bool BuildNets();
		bool BuildPackages();
		bool BuildAllParts();
		bool BuildParts(const FileModel::Design::ComponentsFile* pComponentsFile);
		bool BuildAllComponents();
		bool BuildComponents(const FileModel::Design::ComponentsFile* pComponentsFile);	
		bool BuildVias();

		bool BuildPlacementsFromComponentsFiles();
		bool BuildPlacementsFromComponentsFile(const FileModel::Design::ComponentsFile* pComponentsFile);		

		bool BuildNoneNet();
		bool BreakSinglePinNets();

	// helper convienience methods
	bool CreatePinConnection(const std::string& refDes, unsigned int netNumber, unsigned int pinNumber, const std::string& pinName, unsigned int subnetNumber = 0);

	// Serializes the derived Connectivity view (connectivity.proto) from the
	// in-memory model — a serialization of the BuildPlacements result, not a
	// second join. Called by to_protobuf in BOTH flavours (decision D4:
	// always-on, never gated on includeNormalizedLists).
	void FillConnectivityMessage(Protobuf::ProductModel::Connectivity* pConnectivityMsg) const;
		bool CreateNetConnections(const std::shared_ptr<FileModel::Design::EdaDataFile::NetRecord>& pNetRecord, const std::shared_ptr<FileModel::Design::StepDirectory>& pStepDirectory);

		constexpr inline static const char* NONE_NET_NAME = "$NONE$";

		constexpr inline static bool CLIP_FILEMODEL_AFTER_BUILD = false;

	};	
}
