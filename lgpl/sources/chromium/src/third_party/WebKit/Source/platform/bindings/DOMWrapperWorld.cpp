/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/bindings/DOMWrapperWorld.h"

#include <memory>

#include "platform/bindings/DOMDataStore.h"
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/wtf/HashTraits.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/StdLibExtras.h"

namespace blink {

unsigned DOMWrapperWorld::number_of_non_main_worlds_in_main_thread_ = 0;

// This does not contain the main world because the WorldMap needs
// non-default hashmap traits (WTF::UnsignedWithZeroKeyHashTraits) to contain
// it for the main world's id (0), and it may change the performance trends.
// (see https://crbug.com/704778#c6).
using WorldMap = HashMap<int, DOMWrapperWorld*>;
static WorldMap& GetWorldMap() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<WorldMap>, map, ());
  return *map;
}

#if DCHECK_IS_ON()
static bool IsIsolatedWorldId(int world_id) {
  return DOMWrapperWorld::kMainWorldId < world_id &&
         world_id < DOMWrapperWorld::kIsolatedWorldIdLimit;
}

static bool IsMainWorldId(int world_id) {
  return world_id == DOMWrapperWorld::kMainWorldId;
}
#endif

PassRefPtr<DOMWrapperWorld> DOMWrapperWorld::Create(v8::Isolate* isolate,
                                                    WorldType world_type) {
  DCHECK_NE(WorldType::kIsolated, world_type);
  int world_id = GenerateWorldIdForType(world_type);
  if (world_id == kInvalidWorldId)
    return nullptr;
  return AdoptRef(new DOMWrapperWorld(isolate, world_type, world_id));
}

DOMWrapperWorld::DOMWrapperWorld(v8::Isolate* isolate,
                                 WorldType world_type,
                                 int world_id)
    : world_type_(world_type),
      world_id_(world_id),
      dom_data_store_(
          WTF::WrapUnique(new DOMDataStore(isolate, IsMainWorld()))) {
  switch (world_type_) {
    case WorldType::kMain:
      // The main world is managed separately from worldMap(). See worldMap().
      break;
    case WorldType::kIsolated:
    case WorldType::kInspectorIsolated:
    case WorldType::kGarbageCollector:
    case WorldType::kRegExp:
    case WorldType::kTesting:
    case WorldType::kForV8ContextSnapshotNonMain:
    case WorldType::kWorker: {
      WorldMap& map = GetWorldMap();
      DCHECK(!map.Contains(world_id_));
      map.insert(world_id_, this);
      if (IsMainThread())
        number_of_non_main_worlds_in_main_thread_++;
      break;
    }
  }
}

DOMWrapperWorld& DOMWrapperWorld::MainWorld() {
  DCHECK(IsMainThread());
  DEFINE_STATIC_REF(
      DOMWrapperWorld, cached_main_world,
      (DOMWrapperWorld::Create(v8::Isolate::GetCurrent(), WorldType::kMain)));
  return *cached_main_world;
}

void DOMWrapperWorld::AllWorldsInCurrentThread(
    Vector<RefPtr<DOMWrapperWorld>>& worlds) {
  if (IsMainThread())
    worlds.push_back(&MainWorld());
  for (DOMWrapperWorld* world : GetWorldMap().Values())
    worlds.push_back(world);
}

void DOMWrapperWorld::MarkWrappersInAllWorlds(
    ScriptWrappable* script_wrappable,
    const ScriptWrappableVisitor* visitor) {
  // Marking for worlds other than the main world.
  DCHECK(ThreadState::Current()->GetIsolate());
  for (DOMWrapperWorld* world : GetWorldMap().Values()) {
    DOMDataStore& data_store = world->DomDataStore();
    if (data_store.ContainsWrapper(script_wrappable))
      data_store.MarkWrapper(script_wrappable);
  }

  // Marking for the main world.
  if (IsMainThread())
    script_wrappable->MarkWrapper(visitor);
}

DOMWrapperWorld::~DOMWrapperWorld() {
  DCHECK(!IsMainWorld());
  if (IsMainThread())
    number_of_non_main_worlds_in_main_thread_--;

  // WorkerWorld should be disposed of before the dtor.
  if (!IsWorkerWorld())
    Dispose();
  DCHECK(!GetWorldMap().Contains(world_id_));
}

void DOMWrapperWorld::Dispose() {
  dom_object_holders_.clear();
  dom_data_store_.reset();
  DCHECK(GetWorldMap().Contains(world_id_));
  GetWorldMap().erase(world_id_);
}

PassRefPtr<DOMWrapperWorld> DOMWrapperWorld::EnsureIsolatedWorld(
    v8::Isolate* isolate,
    int world_id) {
#if DCHECK_IS_ON()
  DCHECK(IsIsolatedWorldId(world_id));
#endif

  WorldMap& map = GetWorldMap();
  auto it = map.find(world_id);
  if (it != map.end()) {
    RefPtr<DOMWrapperWorld> world = it->value;
    DCHECK(world->IsIsolatedWorld());
    DCHECK_EQ(world_id, world->GetWorldId());
    return world;
  }

  return AdoptRef(new DOMWrapperWorld(isolate, WorldType::kIsolated, world_id));
}

typedef HashMap<int, RefPtr<SecurityOrigin>> IsolatedWorldSecurityOriginMap;
static IsolatedWorldSecurityOriginMap& IsolatedWorldSecurityOrigins() {
  DCHECK(IsMainThread());
  DEFINE_STATIC_LOCAL(IsolatedWorldSecurityOriginMap, map, ());
  return map;
}

SecurityOrigin* DOMWrapperWorld::IsolatedWorldSecurityOrigin() {
  DCHECK(this->IsIsolatedWorld());
  IsolatedWorldSecurityOriginMap& origins = IsolatedWorldSecurityOrigins();
  IsolatedWorldSecurityOriginMap::iterator it = origins.find(GetWorldId());
  return it == origins.end() ? 0 : it->value.Get();
}

void DOMWrapperWorld::SetIsolatedWorldSecurityOrigin(
    int world_id,
    PassRefPtr<SecurityOrigin> security_origin) {
#if DCHECK_IS_ON()
  DCHECK(IsIsolatedWorldId(world_id));
#endif
  if (security_origin)
    IsolatedWorldSecurityOrigins().Set(world_id, std::move(security_origin));
  else
    IsolatedWorldSecurityOrigins().erase(world_id);
}

typedef HashMap<int, String> IsolatedWorldHumanReadableNameMap;
static IsolatedWorldHumanReadableNameMap& IsolatedWorldHumanReadableNames() {
  DCHECK(IsMainThread());
  DEFINE_STATIC_LOCAL(IsolatedWorldHumanReadableNameMap, map, ());
  return map;
}

String DOMWrapperWorld::NonMainWorldHumanReadableName() {
  DCHECK(!this->IsMainWorld());
  return IsolatedWorldHumanReadableNames().at(GetWorldId());
}

void DOMWrapperWorld::SetNonMainWorldHumanReadableName(
    int world_id,
    const String& human_readable_name) {
#if DCHECK_IS_ON()
  DCHECK(!IsMainWorldId(world_id));
#endif
  IsolatedWorldHumanReadableNames().Set(world_id, human_readable_name);
}

typedef HashMap<int, bool> IsolatedWorldContentSecurityPolicyMap;
static IsolatedWorldContentSecurityPolicyMap&
IsolatedWorldContentSecurityPolicies() {
  DCHECK(IsMainThread());
  DEFINE_STATIC_LOCAL(IsolatedWorldContentSecurityPolicyMap, map, ());
  return map;
}

bool DOMWrapperWorld::IsolatedWorldHasContentSecurityPolicy() {
  DCHECK(this->IsIsolatedWorld());
  IsolatedWorldContentSecurityPolicyMap& policies =
      IsolatedWorldContentSecurityPolicies();
  IsolatedWorldContentSecurityPolicyMap::iterator it =
      policies.find(GetWorldId());
  return it == policies.end() ? false : it->value;
}

void DOMWrapperWorld::SetIsolatedWorldContentSecurityPolicy(
    int world_id,
    const String& policy) {
#if DCHECK_IS_ON()
  DCHECK(IsIsolatedWorldId(world_id));
#endif
  if (!policy.IsEmpty())
    IsolatedWorldContentSecurityPolicies().Set(world_id, true);
  else
    IsolatedWorldContentSecurityPolicies().erase(world_id);
}

void DOMWrapperWorld::RegisterDOMObjectHolderInternal(
    std::unique_ptr<DOMObjectHolderBase> holder_base) {
  DCHECK(!dom_object_holders_.Contains(holder_base.get()));
  holder_base->SetWorld(this);
  holder_base->SetWeak(&DOMWrapperWorld::WeakCallbackForDOMObjectHolder);
  dom_object_holders_.insert(std::move(holder_base));
}

void DOMWrapperWorld::UnregisterDOMObjectHolder(
    DOMObjectHolderBase* holder_base) {
  DCHECK(dom_object_holders_.Contains(holder_base));
  dom_object_holders_.erase(holder_base);
}

void DOMWrapperWorld::WeakCallbackForDOMObjectHolder(
    const v8::WeakCallbackInfo<DOMObjectHolderBase>& data) {
  DOMObjectHolderBase* holder_base = data.GetParameter();
  holder_base->World()->UnregisterDOMObjectHolder(holder_base);
}

// static
int DOMWrapperWorld::GenerateWorldIdForType(WorldType world_type) {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<int>, next_world_id, ());
  if (!next_world_id.IsSet())
    *next_world_id = WorldId::kUnspecifiedWorldIdStart;
  switch (world_type) {
    case WorldType::kMain:
      return kMainWorldId;
    case WorldType::kIsolated:
      // This function should not be called for IsolatedWorld because an
      // identifier for the world is given from out of DOMWrapperWorld.
      NOTREACHED();
      return kInvalidWorldId;
    case WorldType::kInspectorIsolated: {
      DCHECK(IsMainThread());
      static int next_devtools_isolated_world_id =
          IsolatedWorldId::kDevToolsFirstIsolatedWorldId;
      if (next_devtools_isolated_world_id >
          IsolatedWorldId::kDevToolsLastIsolatedWorldId)
        return WorldId::kInvalidWorldId;
      return next_devtools_isolated_world_id++;
    }
    case WorldType::kGarbageCollector:
    case WorldType::kRegExp:
    case WorldType::kTesting:
    case WorldType::kForV8ContextSnapshotNonMain:
    case WorldType::kWorker:
      int world_id = *next_world_id;
      CHECK_GE(world_id, WorldId::kUnspecifiedWorldIdStart);
      *next_world_id = world_id + 1;
      return world_id;
  }
  NOTREACHED();
  return kInvalidWorldId;
}

void DOMWrapperWorld::DissociateDOMWindowWrappersInAllWorlds(
    ScriptWrappable* script_wrappable) {
  DCHECK(script_wrappable);
  DCHECK(IsMainThread());

  script_wrappable->UnsetWrapperIfAny();

  for (auto& world : GetWorldMap().Values())
    world->DomDataStore().UnsetWrapperIfAny(script_wrappable);
}

bool DOMWrapperWorld::HasWrapperInAnyWorldInMainThread(
    ScriptWrappable* script_wrappable) {
  DCHECK(IsMainThread());

  Vector<RefPtr<DOMWrapperWorld>> worlds;
  DOMWrapperWorld::AllWorldsInCurrentThread(worlds);
  for (const auto& world : worlds) {
    DOMDataStore& dom_data_store = world->DomDataStore();
    if (dom_data_store.ContainsWrapper(script_wrappable))
      return true;
  }
  return false;
}

}  // namespace blink
