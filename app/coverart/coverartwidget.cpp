#include "coverartwidget.h"

#include "coverart/online/downloader.h"
#include "icons.h"

#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QPalette>

namespace CoverArt {
  Widget::Widget(QWidget *parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    setMinimumSize(80, 80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // muted, small placeholder text (not the cover pixmap)
    setForegroundRole(QPalette::PlaceholderText);
    QFont f = font();
    if (f.pointSizeF() > 0) {
      f.setPointSizeF(f.pointSizeF() * 0.85);
    }
    setFont(f);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &Widget::showContextMenu);
    auto &downloader = Online::Downloader::instance();
    connect(&downloader, &Online::Downloader::searchStarted, this, &Widget::onSearchStarted);
    connect(&downloader, &Online::Downloader::coverAvailable, this, &Widget::onCoverDownloaded);
    connect(&downloader, &Online::Downloader::searchFinished, this, &Widget::onSearchFinished);
    clear();
  }

  void Widget::setTrack(const Track &track) {
    _track = track;
    render_cover();
  }

  bool Widget::isCurrent(const QString &artist, const QString &album) const {
    return _track.isValid() && _track.artist() == artist && _track.album() == album;
  }

  void Widget::onSearchStarted(const QString &artist, const QString &album) {
    if (isCurrent(artist, album) && _cover_path.isEmpty()) {
      setText(tr("Searching cover art..."));
    }
  }

  void Widget::onCoverDownloaded(const QString &artist, const QString &album, const QString &path) {
    Q_UNUSED(path)
    if (isCurrent(artist, album)) {
      render_cover();
    }
  }

  void Widget::onSearchFinished(const QString &artist, const QString &album) {
    if (isCurrent(artist, album)) {
      render_cover();
    }
  }

  void Widget::render_cover() {
    const QString path = _track.artCover();
    QPixmap cover(path);
    if (path.isEmpty() || cover.isNull()) {
      _cover_path.clear();
      source = QPixmap();
      _zoom = 1.0;
      // request() may not have run yet, so ask rather than assume.
      const bool searching = Online::Downloader::instance().isSearching(_track.artist(), _track.album());
      setText(searching ? tr("Searching cover art...") : tr("No cover art"));
      return;
    }
    _cover_path = path;
    source = cover;
    _zoom = 1.0;
    render();
  }

  void Widget::clear() {
    _track = Track();
    _cover_path.clear();
    source = QPixmap();
    _zoom = 1.0;
    setText(tr("Nothing playing"));
  }

  void Widget::showContextMenu(const QPoint &pos) {
    if (!_track.isValid()) {
      return;
    }

    QMenu menu(this);

    QAction viewer(tr("Open in external viewer"), &menu);
    viewer.setIcon(Icons::get(Icons::Icon::FolderReveal));
    viewer.setEnabled(!_cover_path.isEmpty());
    connect(&viewer, &QAction::triggered, this, [this]() {
      if (!_cover_path.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(_cover_path));
      }
    });

    QAction info(tr("Track info"), &menu);
    info.setIcon(Icons::get(Icons::Icon::Info));
    connect(&info, &QAction::triggered, this, [this]() {
      emit trackInfoRequested(_track);
    });

    // ponytail: minimal zoom controls
    QMenu *zoomMenu = menu.addMenu(tr("Zoom"));
    QAction fit(tr("Fit"), zoomMenu);
    QAction z100(tr("100%"), zoomMenu);
    QAction z200(tr("200%"), zoomMenu);
    connect(&fit, &QAction::triggered, this, [this]() { _zoom = 1.0; render(); });
    connect(&z100, &QAction::triggered, this, [this]() { _zoom = 1.0; render(); });
    connect(&z200, &QAction::triggered, this, [this]() { _zoom = 2.0; render(); });
    zoomMenu->addAction(&fit);
    zoomMenu->addAction(&z100);
    zoomMenu->addAction(&z200);

    menu.addAction(&viewer);
    menu.addAction(&info);
    menu.addMenu(zoomMenu);
    menu.exec(mapToGlobal(pos));
  }

  void Widget::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    if (!source.isNull()) {
      render();
    }
  }

  void Widget::wheelEvent(QWheelEvent *event) {
    if (source.isNull()) {
      QLabel::wheelEvent(event);
      return;
    }
    // ponytail: wheel zoom, clamp 0.2-4x
    const double step = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    _zoom = qBound(0.2, _zoom * step, 4.0);
    render();
    event->accept();
  }

  void Widget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (!source.isNull() && event->button() == Qt::LeftButton) {
      _zoom = 1.0;
      render();
      event->accept();
      return;
    }
    QLabel::mouseDoubleClickEvent(event);
  }

  void Widget::render() {
    if (source.isNull()) {
      return;
    }
    // ponytail: zoom scales the available size
    QSize target(int(width() * _zoom), int(height() * _zoom));
    if (target.width() < 1) target.setWidth(1);
    if (target.height() < 1) target.setHeight(1);
    setPixmap(source.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
}
