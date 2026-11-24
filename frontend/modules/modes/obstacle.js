import {topics} from "../mqtt/topics.js";
import {createToggleMode} from "./toggle.js";

export default createToggleMode(topics.mode.obstacle);
