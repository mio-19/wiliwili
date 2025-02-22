//
// Created by fang on 2023/1/3.
//

#include "activity/player_activity.hpp"
#include "fragment/player_collection.hpp"
#include "fragment/player_coin.hpp"
#include "fragment/player_single_comment.hpp"
#include "view/qr_image.hpp"
#include "view/video_view.hpp"
#include "view/grid_dropdown.hpp"
#include "utils/config_helper.hpp"
#include "utils/dialog_helper.hpp"
#include "presenter/comment_related.hpp"

class DataSourceCommentList : public RecyclingGridDataSource,
                              public CommentRequest {
public:
    DataSourceCommentList(bilibili::VideoCommentListResult result, size_t aid)
        : dataList(result), aid(aid) {}
    RecyclingGridItem* cellForRow(RecyclingGrid* recycler,
                                  size_t index) override {
        if (index == 0) {
            VideoCommentReply* item =
                (VideoCommentReply*)recycler->dequeueReusableCell("Reply");
            item->setHeight(40);
            item->setFocusable(true);
            return item;
        }

        //从缓存列表中取出 或者 新生成一个表单项
        VideoComment* item =
            (VideoComment*)recycler->dequeueReusableCell("Cell");

        item->setData(this->dataList[index - 1]);
        return item;
    }

    size_t getItemCount() override { return dataList.size() + 1; }

    void onItemSelected(RecyclingGrid* recycler, size_t index) override {
        if (index == 0) {
            if (!DialogHelper::checkLogin()) return;
            // 回复评论
            brls::Application::getImeManager()->openForText(
                [this, recycler](std::string text) {
                    this->commentReply(
                        text, aid, 0, 0,
                        [this, recycler](
                            const bilibili::VideoCommentAddResult& result) {
                            this->dataList.insert(dataList.begin(),
                                                  result.reply);
                            recycler->reloadData();
                        });
                },
                "", "", 500, "", 0);
            return;
        }

        PlayerSingleComment* view = new PlayerSingleComment();
        view->setCommentData(dataList[index - 1]);
        auto container = new brls::AppletFrame(view);
        container->setHeaderVisibility(brls::Visibility::GONE);
        container->setFooterVisibility(brls::Visibility::GONE);
        container->setInFadeAnimation(true);
        brls::Application::pushActivity(new brls::Activity(container));

        VideoComment* item =
            dynamic_cast<VideoComment*>(recycler->getGridItemByIndex(index));
        if (!item) return;

        view->likeStateEvent.subscribe([this, item, index](bool value) {
            auto& itemData  = dataList[index - 1];
            itemData.action = value;
            item->setData(itemData);
        });
        view->likeNumEvent.subscribe([this, item, index](size_t value) {
            auto& itemData = dataList[index - 1];
            itemData.like  = value;
            item->setData(itemData);
        });
        view->replyNumEvent.subscribe([this, item, index](size_t value) {
            auto& itemData  = dataList[index - 1];
            itemData.rcount = value;
            item->setData(itemData);
        });
        view->deleteEvent.subscribe([this, recycler, index]() {
            dataList.erase(dataList.begin() + index - 1);
            recycler->setDefaultCellFocus(0);
            recycler->reloadData();
        });
    }

    void appendData(const bilibili::VideoCommentListResult& data) {
        bool skip = false;
        for (auto& i : data) {
            skip = false;
            for (auto& j : this->dataList) {
                if (j.rpid == i.rpid) {
                    skip = true;
                    break;
                }
            }
            if (!skip) this->dataList.push_back(i);
        }
    }

    void clearData() override { this->dataList.clear(); }

private:
    bilibili::VideoCommentListResult dataList;
    size_t aid;
};

class QualityCell : public RecyclingGridItem {
public:
    QualityCell() {
        this->inflateFromXMLRes("xml/views/player_quality_cell.xml");
    }

    void setSelected(bool selected) {
        brls::Theme theme = brls::Application::getTheme();

        this->selected = selected;
        this->checkbox->setVisibility(selected ? brls::Visibility::VISIBLE
                                               : brls::Visibility::GONE);
        this->title->setTextColor(selected
                                      ? theme["brls/list/listItem_value_color"]
                                      : theme["brls/text"]);
    }

    bool getSelected() { return this->selected; }

    BRLS_BIND(brls::Label, title, "cell/title");
    BRLS_BIND(brls::Box, loginLabel, "cell/login");
    BRLS_BIND(brls::Box, vipLabel, "cell/vip");
    BRLS_BIND(brls::CheckBox, checkbox, "cell/checkbox");

    static RecyclingGridItem* create() { return new GridRadioCell(); }

private:
    bool selected = false;
};

class QualityDataSource : public DataSourceDropdown {
public:
    QualityDataSource(bilibili::VideoUrlResult result, BaseDropdown* view)
        : DataSourceDropdown(view), data(std::move(result)) {
        login = !ProgramConfig::instance().getCSRF().empty();
    }

    RecyclingGridItem* cellForRow(RecyclingGrid* recycler,
                                  size_t index) override {
        QualityCell* item = (QualityCell*)recycler->dequeueReusableCell("Cell");

        int quality = data.accept_quality[index];

        item->loginLabel->setVisibility(brls::Visibility::GONE);
        item->vipLabel->setVisibility(brls::Visibility::GONE);

        if (quality > 80) {
            item->vipLabel->setVisibility(brls::Visibility::VISIBLE);
        } else if (quality > 32) {
            if (!login)
                item->loginLabel->setVisibility(brls::Visibility::VISIBLE);
        }

        auto r = this->data.accept_description[index];
        item->title->setText(this->data.accept_description[index]);
        item->setSelected(index == dropdown->getSelected());
        return item;
    }

    size_t getItemCount() override {
        return std::min(data.accept_quality.size(),
                        data.accept_description.size());
    }

    void clearData() override {}

private:
    bilibili::VideoUrlResult data;
    bool login;
};

/// BasePlayerActivity

void BasePlayerActivity::onContentAvailable() { this->setCommonData(); }

void BasePlayerActivity::setCommonData() {
    // 视频评论
    recyclingGrid->registerCell("Cell",
                                []() { return VideoComment::create(); });

    recyclingGrid->registerCell("Reply",
                                []() { return VideoCommentReply::create(); });

    // 切换右侧Tab
    this->registerAction(
        "上一项", brls::ControllerButton::BUTTON_LT,
        [this](brls::View* view) -> bool {
            tabFrame->focus2LastTab();
            return true;
        },
        true);

    this->registerAction(
        "下一项", brls::ControllerButton::BUTTON_RT,
        [this](brls::View* view) -> bool {
            tabFrame->focus2NextTab();
            return true;
        },
        true);

    // 调整清晰度
    this->registerAction("wiliwili/player/quality"_i18n,
                         brls::ControllerButton::BUTTON_START,
                         [this](brls::View* view) -> bool {
                             this->setVideoQuality();
                             return true;
                         });

    this->btnQR->getParent()->addGestureRecognizer(
        new brls::TapGestureRecognizer(this->btnQR->getParent()));

    this->btnAgree->getParent()->addGestureRecognizer(
        new brls::TapGestureRecognizer(this->btnAgree->getParent()));

    this->btnCoin->getParent()->addGestureRecognizer(
        new brls::TapGestureRecognizer(this->btnCoin->getParent()));

    this->btnFavorite->getParent()->addGestureRecognizer(
        new brls::TapGestureRecognizer(this->btnFavorite->getParent()));

    this->videoUserInfo->addGestureRecognizer(
        new brls::TapGestureRecognizer(this->videoUserInfo));

    this->setRelationButton(false, false, false);

    eventSubscribeID =
        MPVCore::instance().getEvent()->subscribe([this](MpvEventEnum event) {
            // 上一次报告历史记录的时间点
            static int64_t lastProgress = MPVCore::instance().video_progress;
            switch (event) {
                case MpvEventEnum::UPDATE_PROGRESS:
                    // 每15秒同步一次进度
                    if (lastProgress + 15 <
                        MPVCore::instance().video_progress) {
                        lastProgress = MPVCore::instance().video_progress;
                        this->reportCurrentProgress(
                            lastProgress, MPVCore::instance().duration);
                    } else if (MPVCore::instance().video_progress <
                               lastProgress) {
                        // 当前播放时间小于上一次上传历史记录的时间点
                        // 发生于向前拖拽进度的时候，此时重置lastProgress的值
                        lastProgress = MPVCore::instance().video_progress;
                    }
                    break;
                case MpvEventEnum::END_OF_FILE:
                    // 尝试自动加载下一分集
                    // 如果当前最顶层是Dialog就放弃自动播放，因为有可能是用户点开了收藏或者投币对话框
                    {
                        auto stack    = brls::Application::getActivitiesStack();
                        Activity* top = stack[stack.size() - 1];
                        if (!dynamic_cast<BasePlayerActivity*>(top)) {
                            // 判断最顶层是否为video
                            if (!dynamic_cast<VideoView*>(
                                    top->getContentView()->getView("video")))
                                return;
                        }
                        if (AUTO_NEXT_PART) this->onIndexChangeToNext();
                    }
                    break;
                case MpvEventEnum::QUALITY_CHANGE_REQUEST:
                    this->setVideoQuality();
                    break;
                default:
                    break;
            }
        });
}

void BasePlayerActivity::showShareDialog(const std::string link) {
    auto container = new brls::Box(brls::Axis::COLUMN);
    container->setJustifyContent(brls::JustifyContent::CENTER);
    container->setAlignItems(brls::AlignItems::CENTER);
    auto qr = new QRImage();
    qr->setSize(brls::Size(256, 256));
    qr->setImageFromQRContent(link);
    qr->setMargins(20, 10, 10, 10);
    container->addView(qr);
    auto hint = new brls::Label();
    hint->setText("wiliwili/player/qr"_i18n);
    hint->setMargins(0, 10, 10, 10);
    container->addView(hint);
    auto dialog = new brls::Dialog(container);
    dialog->addButton("hints/ok"_i18n, []() {});
    dialog->open();
}

void BasePlayerActivity::showCollectionDialog(int64_t id, int videoType) {
    if (!DialogHelper::checkLogin()) return;
    auto playerCollection = new PlayerCollection(id, videoType);
    auto dialog           = new brls::Dialog(playerCollection);
    dialog->addButton("wiliwili/home/common/save"_i18n, [this, id, videoType,
                                                         playerCollection]() {
        this->addResource(id, videoType, playerCollection->isFavorite(),
                          playerCollection->getAddCollectionList(),
                          playerCollection->getDeleteCollectionList());
    });
    playerCollection->registerAction(
        "", brls::ControllerButton::BUTTON_START,
        [this, id, videoType, playerCollection, dialog](...) {
            this->addResource(id, videoType, playerCollection->isFavorite(),
                              playerCollection->getAddCollectionList(),
                              playerCollection->getDeleteCollectionList());
            dialog->dismiss();
            return true;
        },
        true);
    dialog->open();
}

void BasePlayerActivity::showCoinDialog(size_t aid) {
    if (!DialogHelper::checkLogin()) return;

    if (std::to_string(videoDetailResult.owner.mid) ==
        ProgramConfig::instance().getUserID()) {
        DialogHelper::showDialog("wiliwili/player/coin/own"_i18n);
        return;
    }

    int coins = getCoinTolerate();
    if (coins <= 0) {
        DialogHelper::showDialog("wiliwili/player/coin/run_out"_i18n);
        return;
    }

    auto playerCoin = new PlayerCoin();
    if (coins == 1) playerCoin->hideTwoCoin();
    playerCoin->getSelectEvent()->subscribe([this, playerCoin, aid](int value) {
        this->addCoin(aid, value, playerCoin->likeAtTheSameTime());
    });
    auto dialog = new brls::Dialog(playerCoin);
    dialog->open();
}

void BasePlayerActivity::setVideoQuality() {
    if (this->videoUrlResult.accept_description.empty()) return;

    auto* dropdown = new BaseDropdown(
        "wiliwili/player/quality"_i18n,
        [this](int selected) {
            int code = this->videoUrlResult.accept_quality[selected];
            BasePlayerActivity::defaultQuality = code;
            ProgramConfig::instance().setSettingItem(SettingItem::VIDEO_QUALITY,
                                                     code);

            // 如果未登录选择了大于480P清晰度的视频
            if (ProgramConfig::instance().getCSRF().empty() &&
                defaultQuality > 32) {
                DialogHelper::showDialog("wiliwili/home/common/no_login"_i18n);
                return;
            }

            // 在加载视频时，若设置了进度，会自动向前跳转5秒，
            // 这里提前加上5s用来抵消播放视频时的进度问题。
            setProgress(MPVCore::instance().video_progress + 5);

            // dash
            if (!this->videoUrlResult.dash.video.empty()) {
                // dash格式的视频无需重复请求视频链接，这里简单的设置清晰度即可
                videoUrlResult.quality = BasePlayerActivity::defaultQuality;
                this->onVideoPlayUrl(videoUrlResult);
                return;
            }

            // flv
            auto self = dynamic_cast<PlayerSeasonActivity*>(this);
            if (self) {
                this->requestSeasonVideoUrl(episodeResult.bvid,
                                            episodeResult.cid);
            } else {
                this->requestVideoUrl(videoDetailResult.bvid,
                                      videoDetailPage.cid);
            }
        },
        getQualityIndex());
    auto* recycler = dropdown->getRecyclingList();
    recycler->registerCell("Cell", []() { return new QualityCell(); });
    dropdown->setDataSource(
        new QualityDataSource(this->videoUrlResult, dropdown));

    // 因为触摸的问题 视频组件上开启新的 activity 需要同步执行
    // 不然在某些情况下焦点会错乱
    ASYNC_RETAIN
    brls::sync([ASYNC_TOKEN, dropdown]() {
        ASYNC_RELEASE
        brls::Application::pushActivity(new brls::Activity(dropdown));
    });
}

void BasePlayerActivity::onVideoPlayUrl(
    const bilibili::VideoUrlResult& result) {
    brls::Logger::debug("onVideoPlayUrl quality: {}", result.quality);
    //todo: 播放失败时可以尝试备用播放链接

    // 进度向前回退5秒，避免当前进度过于接近结尾出现一加载就结束的情况
    int progress = this->getProgress() - 5;

    if (!result.dash.video.empty()) {
        // dash
        brls::Logger::debug("Video type: dash");
        for (const auto& i : result.dash.video) {
            // todo: 相同清晰度的码率选择方案
            if (result.quality >= i.id) {
                // 手动设置当前选择的清晰度
                videoUrlResult.quality = i.id;
                for (const auto& j : result.dash.audio) {
                    this->video->setUrl(i.base_url, progress, j.base_url);
                    break;
                }
                break;
            }
        }
    } else {
        // flv
        brls::Logger::debug("Video type: flv");
        if (result.durl.size() == 0) {
            brls::Logger::error("No media");
        } else if (result.durl.size() == 1) {
            this->video->setUrl(result.durl[0].url, progress);
        } else {
            std::vector<EDLUrl> urls;
            for (auto& i : result.durl) {
                urls.emplace_back(EDLUrl(i.url, i.length / 1000.0f));
            }
            this->video->setUrl(urls, progress);
        }
    }

    // 设置mpv事件，更新清晰度
    MPVCore::instance().qualityStr =
        videoUrlResult.accept_description[getQualityIndex()];
    MPVCore::instance().getEvent()->fire(MpvEventEnum::QUALITY_CHANGED);

    brls::Logger::debug("BasePlayerActivity::onVideoPlayUrl done");
}

void BasePlayerActivity::onCommentInfo(
    const bilibili::VideoCommentResultWrapper& result) {
    DataSourceCommentList* datasource =
        dynamic_cast<DataSourceCommentList*>(recyclingGrid->getDataSource());
    if (result.cursor.prev == 1) {
        // 第一页评论
        //整合置顶评论
        std::vector<bilibili::VideoCommentResult> comments(result.top_replies);
        comments.insert(comments.end(), result.replies.begin(),
                        result.replies.end());
        // 为了加载骨架屏美观，设置为了100，在加载评论时手动修改回来
        this->recyclingGrid->estimatedRowHeight = 420;
        this->recyclingGrid->setDataSource(
            new DataSourceCommentList(comments, this->getAid()));
        if (comments.size() > 1) this->recyclingGrid->selectRowAt(1, false);
        // 设置评论数量提示
        auto item = this->tabFrame->getTab("wiliwili/player/comment"_i18n);
        if (item) item->setSubtitle(wiliwili::num2w(result.cursor.all_count));
    } else if (datasource) {
        // 第N页评论
        datasource->appendData(result.replies);
        recyclingGrid->notifyDataChanged();
    }
}

void BasePlayerActivity::onRequestCommentError(const std::string& error) {
    brls::sync([this, error]() { this->recyclingGrid->setError(error); });
}

void BasePlayerActivity::onVideoOnlineCount(
    const bilibili::VideoOnlineTotal& result) {
    this->videoPeopleLabel->setText(result.total +
                                    "wiliwili/player/current"_i18n);
    this->video->setOnlineCount(result.total + "wiliwili/player/current"_i18n);
}

void BasePlayerActivity::onVideoRelationInfo(
    const bilibili::VideoRelation& result) {
    brls::Logger::debug("onVideoRelationInfo: {} {} {}", result.like,
                        result.coin, result.favorite);
    this->setRelationButton(result.like, result.coin, result.favorite);
}

void BasePlayerActivity::setRelationButton(bool liked, bool coin,
                                           bool favorite) {
    if (liked) {
        btnAgree->setImageFromSVGRes("svg/bpx-svg-sprite-liked-active.svg");
    } else {
        btnAgree->setImageFromSVGRes("svg/bpx-svg-sprite-liked.svg");
    }
    if (coin) {
        btnCoin->setImageFromSVGRes("svg/bpx-svg-sprite-coin-active.svg");
    } else {
        btnCoin->setImageFromSVGRes("svg/bpx-svg-sprite-coin.svg");
    }
    if (favorite) {
        btnFavorite->setImageFromSVGRes(
            "svg/bpx-svg-sprite-collection-active.svg");
    } else {
        btnFavorite->setImageFromSVGRes("svg/bpx-svg-sprite-collection.svg");
    }
}

void BasePlayerActivity::onError(const std::string& error) {
    ASYNC_RETAIN
    brls::sync([ASYNC_TOKEN, error]() {
        ASYNC_RELEASE
        auto dialog = new brls::Dialog(error);
        dialog->setCancelable(false);
        dialog->addButton("hints/ok"_i18n, []() {
            brls::sync([]() { brls::Application::popActivity(); });
        });
        dialog->open();
    });
}

BasePlayerActivity::~BasePlayerActivity() {
    brls::Logger::debug("del BasePlayerActivity");
    // 取消监控mpv
    MPVCore::instance().getEvent()->unsubscribe(eventSubscribeID);
    // 停止视频播放
    this->video->stop();
}