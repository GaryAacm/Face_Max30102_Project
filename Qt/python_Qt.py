import sys
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import subprocess
import threading
import queue
import time
from PyQt5.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QWidget, QSlider
from PyQt5.QtCore import Qt
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.offsetbox import OffsetImage, AnnotationBbox
import matplotlib.image as mpimg

# 设置全局字体，确保支持中文
plt.rcParams['font.sans-serif'] = ['SimHei']  # 使用黑体
plt.rcParams['axes.unicode_minus'] = False  # 正常显示负号

class RealTimePlotWidget(QWidget):
    def __init__(self, process, data_queue, parent=None):
        super(RealTimePlotWidget, self).__init__(parent)

        self.process = process
        self.data_queue = data_queue
        self.time_data = []
        self.red_data = []
        self.ir_data = []
        self.filtered_red_data = []
        self.filtered_ir_data = []
        self.current_red_value = 0
        self.current_ir_value = 0
        self.red_mean = 0
        self.ir_mean = 0
        self.view_start_index = 0  # 用于跟踪视图的开始位置
        self.view_range = 100  # 显示最新的100个数据点
        self.auto_update = True  # 控制是否自动更新

        # 创建图形对象并设置背景颜色为黑色
        self.figure = Figure(facecolor='black')
        self.canvas = FigureCanvas(self.figure)

        # 创建两个子图
        self.ax1 = self.figure.add_subplot(211)  # 第一个子图
        self.ax2 = self.figure.add_subplot(212)  # 第二个子图

        # 设置子图背景颜色为黑色
        self.ax1.set_facecolor('black')
        self.ax2.set_facecolor('black')

        # 加载并放大图像
        logo_img = mpimg.imread('/home/orangepi/Desktop/zjj/logo.png')
        imagebox = OffsetImage(logo_img, zoom=0.2)  # 将 zoom 参数调高以放大图像
        ab = AnnotationBbox(imagebox, (0.5, 1.2), frameon=False,
                            xycoords='axes fraction', boxcoords="axes fraction", pad=0.1)

        self.ax1.add_artist(ab)

        # 初始化图例和样式
        self.ax1.legend(loc='upper right', facecolor='black', edgecolor='white')
        self.ax2.legend(loc='upper right', facecolor='black', edgecolor='white')
        for ax in [self.ax1, self.ax2]:
            ax.set_xlabel('Time (s)', color='white')
            ax.set_ylabel('Value', color='white')
            ax.set_ylim(0, 200000)
            ax.spines['bottom'].set_color('white')
            ax.spines['top'].set_color('white')
            ax.spines['left'].set_color('white')
            ax.spines['right'].set_color('white')
            ax.tick_params(axis='x', colors='white')
            ax.tick_params(axis='y', colors='white')
            ax.grid(True, which='both', color='white', linestyle='--', linewidth=0.5, alpha=0.8)

        # 设置滑块
        # 左侧滑块（Y轴缩放）
        self.slider_left_ax1 = QSlider(Qt.Vertical)  # 控制上图Y轴缩放
        self.slider_left_ax1.setMinimum(1)
        self.slider_left_ax1.setMaximum(10)
        self.slider_left_ax1.setValue(5)
        self.slider_left_ax1.valueChanged.connect(lambda value: self.zoom_y(value, self.ax1))

        self.slider_left_ax2 = QSlider(Qt.Vertical)  # 控制下图Y轴缩放
        self.slider_left_ax2.setMinimum(1)
        self.slider_left_ax2.setMaximum(10)
        self.slider_left_ax2.setValue(5)
        self.slider_left_ax2.valueChanged.connect(lambda value: self.zoom_y(value, self.ax2))

        # 右侧滑块（X轴平移）
        self.slider_right_ax1 = QSlider(Qt.Vertical)  # 控制上图X轴平移
        self.slider_right_ax1.setMaximum(self.view_range)
        self.slider_right_ax1.valueChanged.connect(lambda value: self.on_slider_change(value, self.ax1))
        self.slider_right_ax1.sliderPressed.connect(self.slider_pressed)
        self.slider_right_ax1.sliderReleased.connect(self.slider_released)

        self.slider_right_ax2 = QSlider(Qt.Vertical)  # 控制下图X轴平移
        self.slider_right_ax2.setMaximum(self.view_range)
        self.slider_right_ax2.valueChanged.connect(lambda value: self.on_slider_change(value, self.ax2))
        self.slider_right_ax2.sliderPressed.connect(self.slider_pressed)
        self.slider_right_ax2.sliderReleased.connect(self.slider_released)

        # 下方滑块（X轴缩放）
        self.slider_bottom_ax1 = QSlider(Qt.Horizontal)  # 控制上图X轴缩放
        self.slider_bottom_ax1.setMinimum(1)
        self.slider_bottom_ax1.setMaximum(10)
        self.slider_bottom_ax1.setValue(1)
        self.slider_bottom_ax1.valueChanged.connect(lambda value: self.zoom_x(value, self.ax1))

        self.slider_bottom_ax2 = QSlider(Qt.Horizontal)  # 控制下图X轴缩放
        self.slider_bottom_ax2.setMinimum(1)
        self.slider_bottom_ax2.setMaximum(10)
        self.slider_bottom_ax2.setValue(1)
        self.slider_bottom_ax2.valueChanged.connect(lambda value: self.zoom_x(value, self.ax2))

        # 布局管理
        main_layout = QHBoxLayout()  # 改为水平布局

        # 左侧滑块布局
        left_layout = QVBoxLayout()
        left_layout.addWidget(self.slider_left_ax1)
        left_layout.addWidget(self.slider_left_ax2)

        # 右侧滑块布局
        right_layout = QVBoxLayout()
        right_layout.addWidget(self.slider_right_ax1)
        right_layout.addWidget(self.slider_right_ax2)

        # 图像和滑块的布局
        plot_layout = QVBoxLayout()
        plot_layout.addWidget(self.canvas)
        plot_layout.addWidget(self.slider_bottom_ax1)  # 上图X轴缩放滑块
        plot_layout.addWidget(self.slider_bottom_ax2)  # 下图X轴缩放滑块

        main_layout.addLayout(left_layout)  # 左侧Y轴缩放滑块
        main_layout.addLayout(plot_layout)
        main_layout.addLayout(right_layout)  # 右侧X轴平移滑块

        self.setLayout(main_layout)


        self.frame_count = 0

        # 设置动画
        self.ani = animation.FuncAnimation(self.figure, self.update_plot, interval=10)

        # 用于显示实时数据的文本
        self.textbox1 = self.ax1.text(0.02, 0.95, '', transform=self.ax1.transAxes, fontsize=12, color='white', verticalalignment='top')
        self.textbox2 = self.ax2.text(0.02, 0.95, '', transform=self.ax2.transAxes, fontsize=12, color='white', verticalalignment='top')

    def update_plot(self, frame):
        self.frame_count += 1
        try:
            line = self.data_queue.get_nowait()
        except queue.Empty:
            return

        parts = line.split(' - ')
        for part in parts:
            if 'Red LED' in part and 'IR LED' in part:
                channel_info, red_str, ir_str = part.split(': ')
                red_str = red_str.split(',')[0]
                red = int(red_str)
                ir = int(ir_str)

                # 记录当前时间戳
                current_time = len(self.time_data)
                self.time_data.append(current_time)

                # 更新累积均值
                self.red_data.append(red)
                self.ir_data.append(ir)
                self.red_mean = sum(self.red_data) / len(self.red_data)
                self.ir_mean = sum(self.ir_data) / len(self.ir_data)

                if red >= self.red_mean * 0.9 and red <= self.red_mean * 1.15:
                    self.filtered_red_data.append(red)
                    self.current_red_value  = red
                else:
                    self.filtered_red_data.append(self.red_mean)

                if ir >= self.ir_mean * 0.9 and ir <= self.ir_mean * 1.15: 
                    self.filtered_ir_data.append(ir)
                    self.current_ir_value = ir
                else:
                    self.filtered_ir_data.append(self.ir_mean)

                # 更新滑块范围
                self.slider_right_ax1.setMaximum(max(0, len(self.filtered_red_data) - self.view_range))
                self.slider_right_ax2.setMaximum(max(0, len(self.filtered_ir_data) - self.view_range))

                if self.auto_update:
                    self.view_start_index = max(0, len(self.filtered_red_data) - self.view_range)
                    self.slider_right_ax1.setValue(self.view_start_index)
                    self.slider_right_ax2.setValue(self.view_start_index)

                # 清除之前的内容并添加背景图像
                self.ax1.clear()
                self.ax2.clear()

                # 加载并放大图像
                logo_img = mpimg.imread('/home/orangepi/Desktop/zjj/人工智能学院.png')
                imagebox = OffsetImage(logo_img, zoom=0.2)  # 将 zoom 参数调高以放大图像
                ab = AnnotationBbox(imagebox, (0.5, 1.2), frameon=False,
                                    xycoords='axes fraction', boxcoords="axes fraction", pad=0.1)

                self.ax1.add_artist(ab)

                # 仅显示从 view_start_index 开始的 view_range 个数据点
                end_index = min(self.view_start_index + self.view_range, len(self.filtered_red_data))
                self.ax1.plot(self.time_data[self.view_start_index:end_index], self.filtered_red_data[self.view_start_index:end_index], label='Red LED', color='red')
                self.ax1.plot(self.time_data[self.view_start_index:end_index], self.filtered_ir_data[self.view_start_index:end_index], label='IR LED', color='white')
                self.ax2.plot(self.time_data[self.view_start_index:end_index], self.filtered_red_data[self.view_start_index:end_index], label='Red LED', color='red')
                self.ax2.plot(self.time_data[self.view_start_index:end_index], self.filtered_ir_data[self.view_start_index:end_index], label='IR LED', color='white')

                # 设置子图的图例和标题
                for ax in [self.ax1, self.ax2]:
                    ax.legend(loc='upper right', facecolor='black', edgecolor='white')
                    ax.set_xlabel('Time (s)', color='white')
                    ax.set_ylabel('Value', color='white')
                    ax.set_ylim(0, 200000)
                    ax.spines['bottom'].set_color('white')
                    ax.spines['top'].set_color('white')
                    ax.spines['left'].set_color('white')
                    ax.spines['right'].set_color('white')
                    ax.tick_params(axis='x', colors='white')
                    ax.tick_params(axis='y', colors='white')
                    ax.grid(True, which='both', color='white', linestyle='--', linewidth=0.5, alpha=0.5)
                    ax.set_xlim(self.time_data[max(0, end_index - self.view_range)], self.time_data[end_index - 1])  # 设定X轴为最新的100个数据点

                # 在右上角显示当前 Red 和 IR 数据以及颜色对应关系
                textstr = f'Red LED (Red Line): {self.current_red_value}\nIR LED (White Line): {self.current_ir_value}'
                props = dict(boxstyle='round', facecolor='black', alpha=0.5)
                self.ax1.text(0.9, 0.9, textstr, transform=self.ax1.transAxes, fontsize=12,
                             verticalalignment='top', horizontalalignment='right', color='white', bbox=props)
                
                self.ax2.text(0.9, 0.9, textstr, transform=self.ax2.transAxes, fontsize=12,
                             verticalalignment='top', horizontalalignment='right', color='white', bbox=props)

                # 更新画布
                self.canvas.draw()

    def zoom_x(self, factor, ax):
        """
        根据滑块的位置放大或缩小X轴图像
        """
        xlim = ax.get_xlim()
        midpoint = (xlim[1] + xlim[0]) / 2
        width = (xlim[1] - xlim[0]) / factor
        new_xlim = (midpoint - width / 2, midpoint + width / 2)

        ax.set_xlim(new_xlim)
        self.canvas.draw()

    def zoom_y(self, factor, ax):
        """
        根据滑块的位置放大或缩小Y轴图像
        """
        ylim = ax.get_ylim()
        midpoint = (ylim[1] + ylim[0]) / 2
        range_y = (ylim[1] - ylim[0]) / (factor / 1.6)  # 调整缩放因子，使得变化更加明显
        new_ylim = (midpoint - range_y / 2, midpoint + range_y / 2)

        ax.set_ylim(new_ylim)
        self.canvas.draw()

    def on_slider_change(self, value, ax):
        if not self.auto_update:  # 只有在自动更新关闭时才会手动更新视图
            self.view_start_index = value
            self.canvas.draw()

    def slider_pressed(self):
        self.auto_update = False

    def slider_released(self):
        self.auto_update = True

class MainWindow(QMainWindow):
    def __init__(self, process, queue):
        super(MainWindow, self).__init__()

        self.setWindowTitle('Real-time Plotting')
        self.setGeometry(100, 100, 800, 600)

        # 嵌入实时绘图部件
        self.plot_widget = RealTimePlotWidget(process, queue, self)
        self.setCentralWidget(self.plot_widget)

        # 添加全屏功能
        self.showFullScreen()

    def closeEvent(self, event):
        # 在关闭窗口时终止C程序
        self.plot_widget.process.terminate()
        event.accept()

if __name__ == "__main__":
    # 启动C程序并读取其输出
    proc = subprocess.Popen(['./max30102'], stdout=subprocess.PIPE, text=True)

    # 创建队列和线程
    data_queue = queue.Queue()
    thread = threading.Thread(target=lambda q, p: [q.put(line.strip()) for line in iter(p.stdout.readline, '')], args=(data_queue, proc))
    thread.start()

    # 创建PyQt5应用程序
    app = QApplication(sys.argv)
    main_window = MainWindow(proc, data_queue)
    main_window.show()

    # 运行PyQt5事件循环
    sys.exit(app.exec_())
