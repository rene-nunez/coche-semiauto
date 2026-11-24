import {topics} from "../mqtt/topics.js";
import {createToggleMode} from "./toggle.js";

export default createToggleMode(topics.modo.seguidor);
