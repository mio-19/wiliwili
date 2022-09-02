#include <nlohmann/json.hpp>
#include <utility>

#include "bilibili.h"
#include "bilibili/util/http.hpp"
#include "bilibili/result/video_detail_result.h"
#include "bilibili/result/home_live_result.h"


namespace bilibili {

    void BilibiliClient::get_video_detail(const std::string& bvid,
                                          const std::function<void(VideoDetailResult)>& callback,
                                          const ErrorCallback& error){
            HTTP::getResultAsync<VideoDetailResult>(Api::Detail, {{"bvid", bvid}}, callback, error);
    }

    void BilibiliClient::get_video_detail(const int aid,
                                          const std::function<void(VideoDetailResult)>& callback,
                                          const ErrorCallback& error){
            HTTP::getResultAsync<VideoDetailResult>(Api::Detail,
                            {{"aid",std::to_string(aid)}},
                            callback, error);
    }

    void BilibiliClient::get_video_detail_all(const std::string& bvid,
                                     const std::function<void(VideoDetailAllResult)>& callback,
                                     const ErrorCallback& error){
        HTTP::getResultAsync<VideoDetailAllResult>(Api::DetailAll, {{"bvid", bvid}}, callback, error);
    }

    void BilibiliClient::get_video_pagelist(const std::string& bvid,
                                          const std::function<void(VideoDetailPageListResult Result)>& callback,
                                          const ErrorCallback& error){
            HTTP::getResultAsync<VideoDetailPageListResult>(Api::PlayPageList,
                                 {{"bvid", std::string(bvid)}},
                                 callback, error);
    }

    void BilibiliClient::get_video_pagelist(const int aid,
                                          const std::function<void(VideoDetailPageListResult)>& callback,
                                          const ErrorCallback& error){
            HTTP::getResultAsync<VideoDetailPageListResult>(Api::PlayPageList,
                            {{"aid",std::to_string(aid)}},
                            callback, error);
    }


    void BilibiliClient::get_video_url(const std::string& bvid, const int cid, const int qn,
                                       const std::function<void(VideoUrlResult)> &callback,
                                       const ErrorCallback &error) {
            HTTP::getResultAsync<VideoUrlResult>(Api::PlayInformation, {
                                    {"bvid",  std::string(bvid)},
                                    {"cid",   std::to_string(cid)},
                                    {"qn",    std::to_string(qn)},
                                    {"fourk", "1"},
                                    {"fnval", "128"},
                                    {"fnver", "0"}},
                            callback, error);
    }

    void BilibiliClient::get_video_url(const int aid, const int cid, const int qn,
                                       const std::function<void(VideoUrlResult)> &callback,
                                       const ErrorCallback &error) {
            HTTP::getResultAsync<VideoUrlResult>(Api::PlayInformation, {
                                         {"aid",   std::to_string(aid)},
                                         {"cid",   std::to_string(cid)},
                                         {"qn",    std::to_string(qn)},
                                         {"fourk", "1"},
                                         {"fnval", "128"},
                                         {"fnver", "0"}},
                                 callback, error);
    }

    void BilibiliClient::get_comment(int aid, int next, int mode,
                            const std::function<void(VideoCommentResultWrapper)>& callback,
                            const ErrorCallback& error){
        HTTP::getResultAsync<VideoCommentResultWrapper>(Api::Comment, {
                                                     {"mode",   std::to_string(mode)},
                                                     {"next",   std::to_string(next)},
                                                     {"oid",    std::to_string(aid)},
                                                     {"plat", "1"},
                                                     {"type", "1"}},
                                             callback, error);
    }

    void BilibiliClient::get_season_detail(const int seasonID, const int epID,
                                          const std::function<void(SeasonResultWrapper)>& callback,
                                          const ErrorCallback& error){
        cpr::Parameters params;
        if(epID == 0){
            params = {{"season_id",std::to_string(seasonID)}};
        }else{
            params = {{"ep_id",std::to_string(epID)}};
        }

        HTTP::getResultAsync<SeasonResultWrapper>(Api::SeasonDetail, params, callback, error);
    }


    void BilibiliClient::get_season_url(const int cid, const int qn,
                        const std::function<void(VideoUrlResult)>& callback,
                        const ErrorCallback& error){
        HTTP::getResultAsync<VideoUrlResult>(Api::SeasonUrl, {
                                                     {"cid",   std::to_string(cid)},
                                                     {"qn",    std::to_string(qn)},
                                                     {"fourk", "1"},
                                                     {"fnver", "0"}},
                                             callback, error);

    }

    void BilibiliClient::get_live_url(const int roomid,
                             const std::function<void(LiveUrlResultWrapper)>& callback,
                             const ErrorCallback& error){
        HTTP::getResultAsync<LiveUrlResultWrapper>(Api::LiveUrl, {{"cid", std::to_string(roomid)}}, callback, error);
    }

    /// 视频页 获取单个视频播放人数
    void BilibiliClient::get_video_online(int aid, int cid,
                                 const std::function<void(VideoOnlineTotal)>& callback,
                                 const ErrorCallback& error){
        HTTP::getResultAsync<VideoOnlineTotal>(Api::OnlineViewerCount,
                                               {
                                                    {"cid", std::to_string(cid)},
                                                    {"aid", std::to_string(aid)},
                                                }, callback, error);
    }

    void BilibiliClient::get_video_online(const std::string& bvid, int cid,
                                 const std::function<void(VideoOnlineTotal)>& callback,
                                 const ErrorCallback& error){
        HTTP::getResultAsync<VideoOnlineTotal>(Api::OnlineViewerCount,
                                               {
                                                       {"cid", std::to_string(cid)},
                                                       {"bvid", bvid},
                                               }, callback, error);
    }

    /// 视频页 获取点赞/收藏/投屏情况
    void BilibiliClient::get_video_relation(const std::string& bvid,
                                   const std::function<void(VideoRelation)>& callback,
                                   const ErrorCallback& error){
        HTTP::getResultAsync<VideoRelation>(Api::VideoRelation, {{"bvid", bvid}}, callback, error);
    }

    void BilibiliClient::get_danmaku(const uint cid,
                            const std::function<void(std::string)>& callback,
                            const ErrorCallback& error){
        cpr::GetCallback<>([callback, error](cpr::Response r) {
                             try{
                                callback(r.text);
                             }
                             catch(const std::exception& e){
                                 ERROR("Network error. [Status code: " + std::to_string(r.status_code) + " ]", -404);
                                 printf("data: %s\n", r.text.c_str());
                                 printf("ERROR: %s\n",e.what());
                             }
                         },
                         cpr::Url{Api::VideoDanmaku},
                         HTTP::HEADERS,
                         cpr::Parameters({{"oid", std::to_string(cid)}}),
                         HTTP::COOKIES,
                         cpr::Timeout{HTTP::TIMEOUT});

    }

    /// 视频页 上报历史记录
    void BilibiliClient::report_history(const std::string& mid, const std::string& access_key,
                                   uint aid, uint cid, int type, uint progress, uint sid, uint epid,
                                   const std::function<void()>& callback,
                                   const ErrorCallback& error){

        cpr::Payload payload = {
                {"mid", mid},
                {"access_key", access_key},
                {"aid", std::to_string(aid)},
                {"cid", std::to_string(cid)},
                {"progress", std::to_string(progress)},
                {"type", std::to_string(type)},
                {"dt", "3"},
        };
        if(type == 4){
            if(sid != 0)
                payload.Add({"sid", std::to_string(sid)});
            if(epid != 0)
                payload.Add({"epid", std::to_string(epid)});
        }

        HTTP::__cpr_post(Api::ProgressReport,{}, payload, [callback, error](const cpr::Response& r){
                if(r.status_code != 200){
                    ERROR("ERROOR: report_history: status_code: " + std::to_string(r.status_code), r.status_code);
                }else{
                    callback();
                }
        });
    }
}