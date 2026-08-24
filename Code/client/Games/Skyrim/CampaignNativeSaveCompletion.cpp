#include <TiltedOnlinePCH.h>

#include <CampaignNativeSave.h>
#include <CampaignNativeSaveCompletion.h>

#include <cryptopp/sha.h>

#include <array>
#include <condition_variable>
#include <optional>
#include <thread>

struct BSWin32SaveDataSystemUtility
{
    virtual ~BSWin32SaveDataSystemUtility();
    virtual bool CreateSaveDirectory(const char*, bool) = 0;
    virtual errno_t PrepareFileSavePath(
        const char*, char*, bool, bool) = 0;
};

using TGetSaveDataSystemUtility = BSWin32SaveDataSystemUtility*();

namespace
{
using namespace STRE::Campaign;

constexpr auto kCompletionDeadline = std::chrono::milliseconds(
    CampaignNativeSaveCompletion::kDeadlineMilliseconds);
constexpr auto kObservationInterval = std::chrono::milliseconds(100);

TGetSaveDataSystemUtility* s_getSaveDataSystemUtility{};

struct CompletionJob
{
    std::string Identity;
    CampaignNativeSaveCompletionPaths Paths;
    CampaignNativeSaveDetail::RequestSlot* pRequestSlot{};
    std::optional<NativeSaveBundleArtifact> ExpectedArtifact;
};

enum class PathPresence
{
    Missing,
    Present,
    Error
};

enum class ObservationState
{
    NotReady,
    Completed,
    Failed
};

class FileHandle final
{
public:
    explicit FileHandle(const std::string& acPath) noexcept
        : m_handle(CreateFileA(
              acPath.c_str(),
              GENERIC_READ,
              FILE_SHARE_READ,
              nullptr,
              OPEN_EXISTING,
              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
              nullptr))
    {
    }

    ~FileHandle()
    {
        if (m_handle != INVALID_HANDLE_VALUE)
            CloseHandle(m_handle);
    }

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    [[nodiscard]] bool IsValid() const noexcept
    {
        return m_handle != INVALID_HANDLE_VALUE;
    }

    [[nodiscard]] HANDLE Get() const noexcept { return m_handle; }

private:
    HANDLE m_handle{INVALID_HANDLE_VALUE};
};

const char* RoleName(NativeSaveMemberRole aRole) noexcept
{
    switch (aRole)
    {
    case NativeSaveMemberRole::Ess:
        return "ess";
    case NativeSaveMemberRole::Skse:
        return "skse";
    }
    return "unknown";
}

PathPresence InspectPath(const std::string& acPath) noexcept
{
    if (GetFileAttributesA(acPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        return PathPresence::Present;
    const DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
        return PathPresence::Missing;
    return PathPresence::Error;
}

bool ResolvePaths(
    const std::string& acIdentity,
    CampaignNativeSaveCompletionPaths& aPaths,
    std::string& aFailureReason)
{
    const std::vector<NativeSaveMemberExpectation> expected =
        BuildExpectedNativeSaveMembers(acIdentity);
    if (expected.size() != 2 || !s_getSaveDataSystemUtility)
    {
        aFailureReason = "save-path-resolver-unavailable";
        return false;
    }
    BSWin32SaveDataSystemUtility* const pUtility =
        s_getSaveDataSystemUtility();
    if (!pUtility)
    {
        aFailureReason = "save-path-utility-unavailable";
        return false;
    }
    std::array<char, 0x104> resolved{};
    if (pUtility->PrepareFileSavePath(
            expected[0].FileName.c_str(),
            resolved.data(),
            false,
            false) != 0 ||
        resolved[0] == '\0')
    {
        aFailureReason = "save-path-resolution-failed";
        return false;
    }
    std::filesystem::path essPath(resolved.data());
    if (!essPath.is_absolute() ||
        _stricmp(
            essPath.filename().string().c_str(),
            expected[0].FileName.c_str()) != 0)
    {
        aFailureReason = "native-identity-path-mismatch";
        return false;
    }
    std::filesystem::path sksePath = essPath;
    sksePath.replace_extension(".skse");
    if (_stricmp(
            sksePath.filename().string().c_str(),
            expected[1].FileName.c_str()) != 0)
    {
        aFailureReason = "cosave-identity-path-mismatch";
        return false;
    }
    aPaths.Ess = essPath.string();
    aPaths.Skse = sksePath.string();
    aPaths.EssTemporary = aPaths.Ess + ".tmp";
    return true;
}

bool TargetsAreFresh(
    const CampaignNativeSaveCompletionPaths& acPaths,
    std::string& aFailureReason) noexcept
{
    for (const std::string* pPath :
         {&acPaths.Ess, &acPaths.Skse, &acPaths.EssTemporary})
    {
        const PathPresence presence = InspectPath(*pPath);
        if (presence == PathPresence::Present)
        {
            aFailureReason = "save-target-already-exists";
            return false;
        }
        if (presence == PathPresence::Error)
        {
            aFailureReason = "save-target-inspection-failed";
            return false;
        }
    }
    return true;
}

ObservationState HashOpenFile(
    HANDLE aHandle,
    NativeSaveMemberRole aRole,
    NativeSaveBundleMember& aMember,
    std::string& aFailureReason) noexcept
{
    LARGE_INTEGER sizeBefore{};
    if (!GetFileSizeEx(aHandle, &sizeBefore))
    {
        aFailureReason = "member-size-read-failed";
        return ObservationState::Failed;
    }
    if (sizeBefore.QuadPart <= 0)
        return ObservationState::NotReady;

    LARGE_INTEGER start{};
    if (!SetFilePointerEx(aHandle, start, nullptr, FILE_BEGIN))
    {
        aFailureReason = "member-seek-failed";
        return ObservationState::Failed;
    }

    try
    {
        CryptoPP::SHA256 hash;
        std::array<std::uint8_t, 256 * 1024> buffer{};
        std::uint64_t total{};
        while (true)
        {
            DWORD bytesRead{};
            if (!ReadFile(
                    aHandle,
                    buffer.data(),
                    static_cast<DWORD>(buffer.size()),
                    &bytesRead,
                    nullptr))
            {
                aFailureReason = "member-read-failed";
                return ObservationState::Failed;
            }
            if (bytesRead == 0)
                break;
            hash.Update(buffer.data(), bytesRead);
            total += bytesRead;
        }

        LARGE_INTEGER sizeAfter{};
        if (!GetFileSizeEx(aHandle, &sizeAfter) ||
            sizeAfter.QuadPart != sizeBefore.QuadPart ||
            total != static_cast<std::uint64_t>(sizeBefore.QuadPart))
        {
            aFailureReason = "member-size-changed-during-hash";
            return ObservationState::Failed;
        }

        aMember.Role = aRole;
        aMember.Size = total;
        hash.Final(aMember.Sha256.data());
        return ObservationState::Completed;
    }
    catch (...)
    {
        aFailureReason = "member-hash-failed";
        return ObservationState::Failed;
    }
}

ObservationState TryObserveCompletion(
    const CompletionJob& acJob,
    std::string& aFailureReason)
{
    if (InspectPath(acJob.Paths.EssTemporary) != PathPresence::Missing)
        return ObservationState::NotReady;

    FileHandle ess(acJob.Paths.Ess);
    if (!ess.IsValid())
        return ObservationState::NotReady;
    FileHandle skse(acJob.Paths.Skse);
    if (!skse.IsValid())
        return ObservationState::NotReady;

    if (InspectPath(acJob.Paths.EssTemporary) != PathPresence::Missing)
        return ObservationState::NotReady;

    std::vector<NativeSaveBundleMember> members(2);
    ObservationState observed = HashOpenFile(
        ess.Get(), NativeSaveMemberRole::Ess, members[0], aFailureReason);
    if (observed != ObservationState::Completed)
        return observed;
    observed = HashOpenFile(
        skse.Get(), NativeSaveMemberRole::Skse, members[1], aFailureReason);
    if (observed != ObservationState::Completed)
        return observed;

    NativeSaveBundleResult artifact = BuildNativeSaveBundleArtifact(
        acJob.Identity, std::move(members));
    if (!artifact)
    {
        aFailureReason = "bundle-metadata-build-failed";
        return ObservationState::Failed;
    }
    if (acJob.ExpectedArtifact &&
        artifact.Value != *acJob.ExpectedArtifact)
    {
        aFailureReason = "existing-save-bundle-conflict";
        return ObservationState::Failed;
    }

    spdlog::info(
        "[STRE][CampaignNativeSave] SAVE_COMPLETION_OBSERVED "
        "nativeSaveIdentity={} evidence=ess-tmp-absent-and-required-members-"
        "opened-readonly-without-write-delete-sharing thread_id={}",
        acJob.Identity,
        GetCurrentThreadId());
    for (const NativeSaveBundleMember& member : artifact.Value.Bundle.Members)
    {
        spdlog::info(
            "[STRE][CampaignNativeSave] SAVE_MEMBER_RESOLVED "
            "nativeSaveIdentity={} role={} size={} thread_id={}",
            acJob.Identity,
            RoleName(member.Role),
            member.Size,
            GetCurrentThreadId());
        spdlog::info(
            "[STRE][CampaignNativeSave] SAVE_MEMBER_HASHED "
            "nativeSaveIdentity={} role={} sha256={} thread_id={}",
            acJob.Identity,
            RoleName(member.Role),
            NativeSaveSha256ToHex(member.Sha256),
            GetCurrentThreadId());
    }
    const std::string fingerprint =
        NativeSaveSha256ToHex(artifact.Value.Fingerprint);
    if (!acJob.pRequestSlot ||
        !acJob.pRequestSlot->Complete(std::move(artifact.Value)))
    {
        aFailureReason = "completion-state-mismatch";
        return ObservationState::Failed;
    }
    spdlog::info(
        "[STRE][CampaignNativeSave] REQUEST_COMPLETED "
        "nativeSaveIdentity={} fingerprint_algorithm={} "
        "fingerprint_version={} fingerprint={} metadata_codec_version={} "
        "thread_id={}",
        acJob.Identity,
        kNativeSaveFingerprintAlgorithm,
        kNativeSaveFingerprintVersion,
        fingerprint,
        kNativeSaveMetadataCodecVersion,
        GetCurrentThreadId());
    return ObservationState::Completed;
}

void MarkFailed(
    const CompletionJob& acJob,
    const std::string& acReason) noexcept
{
    try
    {
        if (acJob.pRequestSlot)
            (void)acJob.pRequestSlot->Fail(acReason);
    }
    catch (...)
    {
    }
    spdlog::error(
        "[STRE][CampaignNativeSave] REQUEST_FAILED "
        "nativeSaveIdentity={} reason={} thread_id={}",
        acJob.Identity,
        acReason,
        GetCurrentThreadId());
}

class CompletionWorker final
{
public:
    CompletionWorker()
        : m_thread([this]() { Run(); })
    {
    }

    ~CompletionWorker()
    {
        {
            std::lock_guard lock(m_mutex);
            m_stopping = true;
        }
        m_condition.notify_all();
        if (m_thread.joinable())
            m_thread.join();
    }

    CompletionWorker(const CompletionWorker&) = delete;
    CompletionWorker& operator=(const CompletionWorker&) = delete;

    [[nodiscard]] bool Start(CompletionJob aJob)
    {
        std::lock_guard lock(m_mutex);
        if (m_stopping || m_nextJob)
            return false;
        m_nextJob = std::move(aJob);
        m_condition.notify_all();
        return true;
    }

private:
    void Run() noexcept
    {
        while (true)
        {
            std::optional<CompletionJob> job;
            {
                std::unique_lock lock(m_mutex);
                m_condition.wait(
                    lock, [this]() { return m_stopping || m_nextJob; });
                if (m_stopping)
                    return;
                job = std::move(m_nextJob);
                m_nextJob.reset();
            }
            Observe(*job);
        }
    }

    void Observe(const CompletionJob& acJob) noexcept
    {
        const auto deadline = std::chrono::steady_clock::now() +
            kCompletionDeadline;
        while (true)
        {
            try
            {
                std::string failureReason;
                const ObservationState state = TryObserveCompletion(
                    acJob, failureReason);
                if (state == ObservationState::Completed)
                    return;
                if (state == ObservationState::Failed)
                {
                    MarkFailed(acJob, failureReason);
                    return;
                }
            }
            catch (...)
            {
                MarkFailed(acJob, "completion-observer-failed");
                return;
            }

            if (std::chrono::steady_clock::now() >= deadline)
            {
                MarkFailed(acJob, "completion-timeout");
                return;
            }

            std::unique_lock lock(m_mutex);
            if (m_condition.wait_for(
                    lock,
                    kObservationInterval,
                    [this]() { return m_stopping; }))
            {
                return;
            }
        }
    }

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::optional<CompletionJob> m_nextJob;
    bool m_stopping{};
    std::thread m_thread;
};

CompletionWorker& GetCompletionWorker()
{
    static CompletionWorker s_worker;
    return s_worker;
}
}

bool CampaignNativeSaveCompletion::IsAvailable() noexcept
{
    return s_getSaveDataSystemUtility != nullptr;
}

bool CampaignNativeSaveCompletion::PrepareFresh(
    const std::string& acIdentity,
    CampaignNativeSaveCompletionPaths& aPaths,
    std::string& aFailureReason)
{
    return ResolvePaths(acIdentity, aPaths, aFailureReason) &&
        TargetsAreFresh(aPaths, aFailureReason);
}

bool CampaignNativeSaveCompletion::PrepareExisting(
    const std::string& acIdentity,
    CampaignNativeSaveCompletionPaths& aPaths,
    std::string& aFailureReason)
{
    if (!ResolvePaths(acIdentity, aPaths, aFailureReason))
        return false;
    if (InspectPath(aPaths.EssTemporary) != PathPresence::Missing ||
        InspectPath(aPaths.Ess) != PathPresence::Present ||
        InspectPath(aPaths.Skse) != PathPresence::Present)
    {
        aFailureReason = "existing-save-bundle-incomplete";
        return false;
    }
    return true;
}

bool CampaignNativeSaveCompletion::Start(
    std::string aIdentity,
    CampaignNativeSaveCompletionPaths aPaths,
    CampaignNativeSaveDetail::RequestSlot& aRequestSlot,
    std::optional<NativeSaveBundleArtifact> aExpectedArtifact)
{
    return GetCompletionWorker().Start(
        {std::move(aIdentity), std::move(aPaths), &aRequestSlot,
         std::move(aExpectedArtifact)});
}

static TiltedPhoques::Initializer s_campaignNativeSaveCompletionPath(
    []()
    {
        // CommonLibSSE-NG BSWin32SaveDataSystemUtility::GetSingleton, verified
        // against the installed AE 1.6.1170 Address Library.
        POINTER_SKYRIMSE(
            TGetSaveDataSystemUtility,
            getSaveDataSystemUtility,
            109278);
        s_getSaveDataSystemUtility = getSaveDataSystemUtility.Get();
        if (!s_getSaveDataSystemUtility)
        {
            spdlog::error(
                "[STRE][CampaignNativeSave] COMPLETION_UNAVAILABLE "
                "path_resolver_resolved=false");
            return;
        }
        spdlog::info(
            "[STRE][CampaignNativeSave] COMPLETION_CONFIGURED "
            "path_resolver_relocation=109278");
    });
